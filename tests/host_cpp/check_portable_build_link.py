#!/usr/bin/env python3
"""Build and whole-link the six ESP-IDF-free portable components on Linux."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tempfile
import time


COMPONENTS = {
    "wfrac_protocol": {
        "sources": (
            "src/crc16.cpp", "src/aircon_frame.cpp", "src/aircon_state.cpp",
            "src/capabilities.cpp", "src/operation_data.cpp", "src/aircon_block.cpp",
            "src/base64.cpp", "src/aircon_stat.cpp",
        ),
        "requires": (),
    },
    "wfrac_device_model": {
        "sources": (
            "src/protocol_mapping.cpp", "src/device_state.cpp",
            "src/protocol_command_mapping.cpp", "src/protocol_request_mapping.cpp",
            "src/protocol_observation.cpp",
        ),
        "requires": ("wfrac_protocol",),
    },
    "wfrac_api": {
        "sources": ("src/api_request.cpp", "src/api_response.cpp"),
        "requires": ("wfrac_protocol",),
    },
    "wfrac_transport": {
        "sources": ("src/scheme_policy.cpp",),
        "requires": (),
    },
    "wfrac_client": {
        "sources": (
            "src/request_executor.cpp", "src/control_verification.cpp",
            "src/runtime_orchestrator.cpp",
        ),
        "requires": ("wfrac_api", "wfrac_device_model", "wfrac_transport"),
    },
    "wfrac_persistence": {
        "sources": ("src/records.cpp",),
        "requires": ("wfrac_protocol", "wfrac_transport"),
    },
}
REGISTER_KEYS = {"SRCS", "INCLUDE_DIRS", "REQUIRES"}
TOKEN_RE = re.compile(r'"([^"\\]*(?:\\.[^"\\]*)*)"|([^\s]+)')
TOTAL_TIMEOUT_SECONDS = 900.0
COMMAND_TIMEOUT_SECONDS = 30.0
LINK_TIMEOUT_SECONDS = 60.0
DIAGNOSTIC_LIMIT = 6000


class BuildFailure(Exception):
    """A fail-closed manifest, toolchain, compile, archive, link, or timeout error."""


def checked_relative(value: str, *, expected_prefix: str | None = None) -> PurePosixPath:
    if (not isinstance(value, str) or not value or "\\" in value or
            value.startswith("/") or any(ord(char) < 32 or ord(char) == 127 for char in value)):
        raise BuildFailure("unsafe or missing repository-relative path")
    path = PurePosixPath(value)
    if any(part in ("", ".", "..") for part in path.parts):
        raise BuildFailure("unsafe repository-relative path: " + value)
    if expected_prefix and (not path.parts or path.parts[0] != expected_prefix):
        raise BuildFailure("path is outside expected " + expected_prefix + " root: " + value)
    return path


def parse_component_register(path: Path) -> dict[str, tuple[str, ...]]:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise BuildFailure(f"cannot read component manifest {path.name}: {exc}") from None
    text = re.sub(r"(?m)^\s*#.*$", "", text)
    matches = list(re.finditer(r"\bidf_component_register\s*\(([^()]*)\)", text, re.S))
    if len(matches) != 1:
        raise BuildFailure("component manifest must contain exactly one simple idf_component_register call")
    match = matches[0]
    if (text[:match.start()].strip() or text[match.end():].strip()):
        raise BuildFailure("unexpected CMake command outside idf_component_register")

    body = match.group(1)
    parsed: dict[str, list[str]] = {}
    current: str | None = None
    cursor = 0
    for token_match in TOKEN_RE.finditer(body):
        if body[cursor:token_match.start()].strip():
            raise BuildFailure("unsupported syntax in idf_component_register")
        token = token_match.group(1) if token_match.group(1) is not None else token_match.group(2)
        cursor = token_match.end()
        if token in REGISTER_KEYS:
            if token in parsed:
                raise BuildFailure("duplicate " + token + " in component manifest")
            parsed[token] = []
            current = token
            continue
        if current is None:
            raise BuildFailure("unexpected value before a supported CMake key")
        if any(char in token for char in ("$", ";", "(", ")", "\\")):
            raise BuildFailure("unsupported expansion or path syntax in component manifest")
        parsed[current].append(token)
    if body[cursor:].strip():
        raise BuildFailure("unsupported trailing syntax in idf_component_register")
    if "SRCS" not in parsed or "INCLUDE_DIRS" not in parsed:
        raise BuildFailure("component manifest lacks SRCS or INCLUDE_DIRS")
    if set(parsed) - REGISTER_KEYS:
        raise BuildFailure("unsupported component registration key")
    return {key: tuple(values) for key, values in parsed.items()}


def enumerate_cpp_sources(root: Path) -> tuple[str, ...]:
    found: list[str] = []

    def visit(directory: Path) -> None:
        try:
            with os.scandir(directory) as scan:
                entries = sorted(scan, key=lambda entry: entry.name)
        except OSError as exc:
            raise BuildFailure(f"cannot enumerate source directory {directory.name}: {exc}") from None
        for entry in entries:
            item = Path(entry.path)
            try:
                if entry.is_symlink() and entry.is_dir(follow_symlinks=True):
                    raise BuildFailure("symlinked source directory is unsupported")
                if entry.is_dir(follow_symlinks=False):
                    visit(item)
                elif entry.name.endswith(".cpp"):
                    if entry.is_symlink() or not entry.is_file(follow_symlinks=False):
                        raise BuildFailure("C++ source must be a regular non-symlink file")
                    found.append(item.relative_to(root.parent).as_posix())
            except OSError as exc:
                raise BuildFailure(f"cannot inspect C++ source {entry.name}: {exc}") from None

    if root.is_symlink() or not root.is_dir():
        raise BuildFailure("component src root is missing, non-directory, or symlinked")
    visit(root)
    return tuple(found)


def resolve_inside(repo_root: Path, relative: str, *, directory: bool) -> Path:
    rel = checked_relative(relative)
    candidate = repo_root.joinpath(*rel.parts)
    try:
        resolved = candidate.resolve(strict=True)
        if not resolved.is_relative_to(repo_root):
            raise BuildFailure("resolved path escapes repository: " + relative)
        if candidate.is_symlink():
            raise BuildFailure("symlinked source/include path is unsupported: " + relative)
        if directory:
            if not candidate.is_dir():
                raise BuildFailure("required include root is not a directory: " + relative)
        elif not candidate.is_file():
            raise BuildFailure("required source/manifest is not a regular file: " + relative)
    except OSError as exc:
        raise BuildFailure(f"cannot resolve required path {relative}: {exc}") from None
    return candidate


def run_capture(command: list[str], *, cwd: Path, env: dict[str, str],
                timeout: float, started: float) -> subprocess.CompletedProcess[str]:
    remaining = TOTAL_TIMEOUT_SECONDS - (time.monotonic() - started)
    if remaining <= 0:
        raise BuildFailure(f"overall campaign watchdog expired after {TOTAL_TIMEOUT_SECONDS:.0f}s")
    effective_timeout = min(timeout, remaining)
    print("COMMAND=" + json.dumps(command, ensure_ascii=True), flush=True)
    try:
        result = subprocess.run(
            command, cwd=cwd, env=env, check=False, capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=effective_timeout,
        )
    except subprocess.TimeoutExpired as exc:
        for label, value in (("STDOUT", exc.stdout), ("STDERR", exc.stderr)):
            if value:
                if isinstance(value, bytes):
                    value = value.decode("utf-8", errors="replace")
                print(label + "_PARTIAL=" + value[-DIAGNOSTIC_LIMIT:], flush=True)
        raise BuildFailure(f"command watchdog expired after {effective_timeout:.1f}s") from None
    except OSError as exc:
        raise BuildFailure(f"cannot execute {command[0]}: {exc}") from None

    print("EXIT_CODE=" + str(result.returncode), flush=True)
    for label, value in (("STDOUT", result.stdout), ("STDERR", result.stderr)):
        if value:
            print(label + "=" + value[-DIAGNOSTIC_LIMIT:], flush=True)
    return result


def run_checked(command: list[str], *, cwd: Path, env: dict[str, str],
                timeout: float, started: float, phase: str) -> subprocess.CompletedProcess[str]:
    result = run_capture(command, cwd=cwd, env=env, timeout=timeout, started=started)
    if result.returncode != 0:
        raise BuildFailure(f"{phase} failed with exit {result.returncode}")
    return result


def dependency_closure(name: str) -> tuple[str, ...]:
    ordered: list[str] = []
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(component: str) -> None:
        if component in visiting:
            raise BuildFailure("cyclic component dependency at " + component)
        if component in visited:
            return
        if component not in COMPONENTS:
            raise BuildFailure("unknown component dependency: " + component)
        visiting.add(component)
        for dependency in COMPONENTS[component]["requires"]:
            visit(dependency)
        visiting.remove(component)
        visited.add(component)
        ordered.append(component)

    visit(name)
    return tuple(ordered)


def validate_manifests(repo_root: Path, started: float, env: dict[str, str]) -> dict[str, list[Path]]:
    source_paths: dict[str, list[Path]] = {}
    expected_tracked: list[str] = []
    cmake_paths: list[str] = []
    total_sources = 0

    for name, expected in COMPONENTS.items():
        component_root = resolve_inside(repo_root, "components/" + name, directory=True)
        cmake_rel = "components/" + name + "/CMakeLists.txt"
        cmake_path = resolve_inside(repo_root, cmake_rel, directory=False)
        cmake_paths.append(cmake_rel)
        actual = parse_component_register(cmake_path)
        expected_sources = expected["sources"]
        if actual.get("SRCS") != expected_sources:
            raise BuildFailure("declared SRCS differ from the reviewed source manifest: " + name)
        if actual.get("INCLUDE_DIRS") != ("include",):
            raise BuildFailure("declared INCLUDE_DIRS differ from the reviewed public root: " + name)
        if actual.get("REQUIRES", ()) != expected["requires"]:
            raise BuildFailure("declared REQUIRES differ from the reviewed dependency manifest: " + name)
        if set(actual) != ({"SRCS", "INCLUDE_DIRS"} | ({"REQUIRES"} if expected["requires"] else set())):
            raise BuildFailure("unexpected component registration fields: " + name)

        include_rel = "components/" + name + "/include"
        include_root = resolve_inside(repo_root, include_rel, directory=True)
        try:
            with os.scandir(include_root) as scan:
                next(scan, None)
        except OSError as exc:
            raise BuildFailure(f"required public include root is inaccessible: {name}: {exc}") from None

        src_root = component_root / "src"
        actual_inventory = enumerate_cpp_sources(src_root)
        if actual_inventory != tuple(sorted(expected_sources)):
            raise BuildFailure("actual .cpp inventory differs from the reviewed source manifest: " + name)

        resolved_sources = []
        for relative_source in expected_sources:
            checked_relative(relative_source, expected_prefix="src")
            if not relative_source.endswith(".cpp"):
                raise BuildFailure("manifest source is not a .cpp file")
            repo_rel = "components/" + name + "/" + relative_source
            source_path = resolve_inside(repo_root, repo_rel, directory=False)
            expected_tracked.append(repo_rel)
            resolved_sources.append(source_path)
        source_paths[name] = resolved_sources
        total_sources += len(resolved_sources)
        print(f"MANIFEST_PASS component={name} sources={len(resolved_sources)} requires="+
              ",".join(expected["requires"]), flush=True)

    if total_sources != 20:
        raise BuildFailure(f"reviewed source manifest count changed: {total_sources}, expected 20")
    tracked_args = ["git", "ls-files", "--error-unmatch", "--", *cmake_paths, *expected_tracked]
    tracked = run_checked(tracked_args, cwd=repo_root, env=env,
                          timeout=COMMAND_TIMEOUT_SECONDS, started=started, phase="tracked source check")
    tracked_paths = tuple(line for line in tracked.stdout.splitlines() if line)
    expected_paths = tuple(sorted(cmake_paths + expected_tracked))
    if tuple(sorted(tracked_paths)) != expected_paths:
        raise BuildFailure("tracked component manifests/source inventory does not match reviewed paths")
    return source_paths


def validate_external_consumer(repo_root: Path, build_root: Path, archives: list[Path],
                               compiler: str, env: dict[str, str], started: float) -> None:
    results = {"SOURCE_COPY": "NOT_RUN", "COMPILE": "NOT_RUN", "STATIC_LINK": "NOT_RUN",
               "MEMBER_MAP": "NOT_RUN", "NEGATIVE_CONTROL": "NOT_RUN"}
    current_stage: str | None = None
    try:
        fixture_rel = "tests/host_cpp/test_portable_external_consumer.cpp"
        fixture = resolve_inside(repo_root, fixture_rel, directory=False)
        consumer_root = build_root / "external-consumer"
        consumer_root.mkdir()
        consumer_source = consumer_root / "consumer.cpp"
        current_stage = "SOURCE_COPY"
        original_bytes = fixture.read_bytes()
        original_digest = hashlib.sha256(original_bytes).hexdigest()
        consumer_source.write_bytes(original_bytes)
        copied_bytes = consumer_source.read_bytes()
        if (len(copied_bytes) != len(original_bytes) or
                hashlib.sha256(copied_bytes).hexdigest() != original_digest):
            raise BuildFailure("external consumer fixture copy identity mismatch")
        print(f"EXTERNAL_CONSUMER_SOURCE path={fixture_rel} bytes={len(original_bytes)} sha256={original_digest}", flush=True)
        results[current_stage] = "PASS"
        current_stage = None

        include_args: list[str] = []
        for name in COMPONENTS:
            include_root = resolve_inside(repo_root, "components/" + name + "/include", directory=True)
            include_args.extend(["-I", str(include_root)])

        consumer_object = consumer_root / "consumer.o"
        current_stage = "COMPILE"
        compile_command = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                           *include_args, "-c", str(consumer_source), "-o", str(consumer_object)]
        run_checked(compile_command, cwd=consumer_root, env=env,
                    timeout=COMMAND_TIMEOUT_SECONDS, started=started,
                    phase="external consumer C++17 compile")
        if not consumer_object.is_file() or consumer_object.stat().st_size == 0:
            raise BuildFailure("consumer compiler succeeded without a nonempty object")
        print(f"EXTERNAL_CONSUMER_COMPILE=PASS object_bytes={consumer_object.stat().st_size}", flush=True)
        results[current_stage] = "PASS"
        current_stage = None

        consumer_executable = consumer_root / "consumer.elf"
        consumer_map = consumer_root / "consumer.map"
        current_stage = "STATIC_LINK"
        link_command = [compiler, str(consumer_object), "-Wl,--start-group",
                        *(str(path) for path in archives), "-Wl,--end-group",
                        "-Wl,-Map=" + str(consumer_map), "-o", str(consumer_executable)]
        run_checked(link_command, cwd=consumer_root, env=env,
                    timeout=LINK_TIMEOUT_SECONDS, started=started,
                    phase="external consumer normal static-library link")
        if (not consumer_executable.is_file() or consumer_executable.stat().st_size == 0 or
                not consumer_map.is_file()):
            raise BuildFailure("consumer link succeeded without a nonempty executable and link map")
        print(f"EXTERNAL_CONSUMER_STATIC_LINK=PASS executable_bytes={consumer_executable.stat().st_size}", flush=True)
        print("EXTERNAL_CONSUMER_EXECUTABLE_NOT_EXECUTED=true", flush=True)
        results[current_stage] = "PASS"
        current_stage = None

        current_stage = "MEMBER_MAP"
        try:
            map_text = consumer_map.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            raise BuildFailure(f"cannot read external consumer link map: {exc}") from None
        expected_members = (
            ("wfrac_protocol", "wfrac_protocol__src__crc16_cpp.o"),
            ("wfrac_device_model", "wfrac_device_model__src__protocol_mapping_cpp.o"),
            ("wfrac_api", "wfrac_api__src__api_response_cpp.o"),
            ("wfrac_transport", "wfrac_transport__src__scheme_policy_cpp.o"),
            ("wfrac_client", "wfrac_client__src__request_executor_cpp.o"),
            ("wfrac_persistence", "wfrac_persistence__src__records_cpp.o"),
        )
        archive_names = {path.name for path in archives}
        if archive_names != {"lib" + name + ".a" for name in COMPONENTS}:
            raise BuildFailure("external consumer archive set differs from the six portable components")
        for component, object_name in expected_members:
            member = f"lib{component}.a({object_name})"
            if member not in map_text:
                raise BuildFailure("consumer link map lacks required archive member: " + member)
            print("EXTERNAL_CONSUMER_MEMBER=" + member, flush=True)
        print("EXTERNAL_CONSUMER_MEMBER_MAP=PASS members=6/6", flush=True)
        results[current_stage] = "PASS"
        current_stage = None

        current_stage = "NEGATIVE_CONTROL"
        negative_archives = [path for path in archives if path.name != "libwfrac_persistence.a"]
        if len(negative_archives) != len(COMPONENTS) - 1:
            raise BuildFailure("negative consumer link did not omit exactly the persistence library")
        negative_executable = consumer_root / "consumer-without-persistence.elf"
        negative_map = consumer_root / "consumer-without-persistence.map"
        negative_command = [compiler, str(consumer_object), "-Wl,--start-group",
                            *(str(path) for path in negative_archives), "-Wl,--end-group",
                            "-Wl,-Map=" + str(negative_map), "-o", str(negative_executable)]
        negative = run_capture(negative_command, cwd=consumer_root, env=env,
                               timeout=LINK_TIMEOUT_SECONDS, started=started)
        expected_symbol = (
            "wfrac_persistence::encode_scheme(wfrac_transport::SchemeKnowledge, "
            "wfrac_persistence::RecordBuffer*)")
        unresolved = re.findall(r"undefined reference to [`']([^`']+)[`']", negative.stderr)
        if negative.returncode == 0:
            raise BuildFailure("negative consumer link unexpectedly succeeded without libwfrac_persistence.a")
        if unresolved != [expected_symbol]:
            raise BuildFailure("negative consumer link did not fail solely on the expected persistence symbol")
        print("EXTERNAL_CONSUMER_NEGATIVE_EXPECTED_SYMBOL=" + expected_symbol, flush=True)
        print("EXTERNAL_CONSUMER_NEGATIVE_CONTROL=PASS missing_library=wfrac_persistence", flush=True)
        results[current_stage] = "PASS"
        current_stage = None

        for name, result in results.items():
            print(f"EXTERNAL_CONSUMER_{name}={result}", flush=True)
        print("EXTERNAL_CONSUMER_RESULT=PASS", flush=True)
    except Exception as exc:
        if current_stage is not None and results[current_stage] == "NOT_RUN":
            results[current_stage] = "FAIL"
        for name, result in results.items():
            print(f"EXTERNAL_CONSUMER_{name}={result}", flush=True)
        print("EXTERNAL_CONSUMER_RESULT=FAIL", flush=True)
        if isinstance(exc, BuildFailure):
            raise
        raise BuildFailure("external consumer validation failed: " + str(exc)) from None


def safe_child_environment(build_root: Path) -> dict[str, str]:
    # Compiler, archiver and linker receive no inherited credentials or CI tokens.
    path = os.environ.get("PATH")
    if not path:
        raise BuildFailure("PATH is unavailable for the native compiler toolchain")
    tmp = str(build_root / "tmp")
    home = str(build_root / "home")
    Path(tmp).mkdir()
    Path(home).mkdir()
    return {"PATH": path, "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HOME": home, "TMPDIR": tmp, "TMP": tmp, "TEMP": tmp}


def main() -> int:
    started = time.monotonic()
    external_consumer_started = False
    try:
        if not sys.platform.startswith("linux"):
            raise BuildFailure("this native portable build/link check requires Linux Host CI")
        repo_root = Path(__file__).resolve().parents[2]
        if not repo_root.is_dir():
            raise BuildFailure("repository root cannot be resolved")

        raw_git = shutil.which("git")
        if not raw_git:
            raise BuildFailure("required local git executable was not found")
        base_env = {"PATH": os.environ.get("PATH", ""), "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8"}
        revision_result = run_checked([raw_git, "-C", str(repo_root), "rev-parse", "HEAD"],
                                      cwd=repo_root, env=base_env, timeout=COMMAND_TIMEOUT_SECONDS,
                                      started=started, phase="target revision query")
        revision = revision_result.stdout.strip()
        if not re.fullmatch(r"[0-9a-f]{40}", revision):
            raise BuildFailure("target revision has an invalid format")
        print("TARGET_SHA=" + revision, flush=True)

        compiler = shutil.which("g++")
        archiver = shutil.which("ar")
        if not compiler or not archiver:
            raise BuildFailure("required native g++ and ar tools must both be on PATH")

        with tempfile.TemporaryDirectory(prefix="portable-build-link-") as temporary:
            build_root = Path(temporary).resolve(strict=True)
            if build_root.is_relative_to(repo_root):
                raise BuildFailure("temporary build output unexpectedly resides inside repository")
            env = safe_child_environment(build_root)
            compiler_version = run_checked([compiler, "--version"], cwd=repo_root, env=env,
                                            timeout=COMMAND_TIMEOUT_SECONDS, started=started,
                                            phase="compiler identity")
            compiler_target = run_checked([compiler, "-dumpmachine"], cwd=repo_root, env=env,
                                          timeout=COMMAND_TIMEOUT_SECONDS, started=started,
                                          phase="compiler target")
            archiver_version = run_checked([archiver, "--version"], cwd=repo_root, env=env,
                                           timeout=COMMAND_TIMEOUT_SECONDS, started=started,
                                           phase="archiver identity")
            print("COMPILER=" + compiler, flush=True)
            print("COMPILER_VERSION=" + next((line for line in compiler_version.stdout.splitlines() if line.strip()), "UNKNOWN"), flush=True)
            print("COMPILER_TARGET=" + compiler_target.stdout.strip(), flush=True)
            print("ARCHIVER=" + archiver, flush=True)
            print("ARCHIVER_VERSION=" + next((line for line in archiver_version.stdout.splitlines() if line.strip()), "UNKNOWN"), flush=True)

            sources = validate_manifests(repo_root, started, env)
            all_objects: dict[str, list[Path]] = {}
            archives: list[Path] = []
            object_manifest: list[tuple[str, str, Path]] = []
            (build_root / "objects").mkdir()
            (build_root / "archives").mkdir()

            for name, component_sources in sources.items():
                component_objects: list[Path] = []
                include_components = dependency_closure(name)
                include_args: list[str] = []
                for dependency in include_components:
                    include_args.extend(["-I", str(repo_root / "components" / dependency / "include")])

                for source_path in component_sources:
                    source_rel = source_path.relative_to(repo_root).as_posix()
                    source_digest = hashlib.sha256(source_path.read_bytes()).hexdigest()
                    object_name = (name + "__" + source_path.relative_to(repo_root / "components" / name)
                                   .as_posix().replace("/", "__").replace(".", "_") + ".o")
                    object_path = build_root / "objects" / object_name
                    command = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-fPIC", "-c",
                               "-iquote", str(source_path.parent), *include_args,
                               str(source_path), "-o", str(object_path)]
                    run_checked(command, cwd=repo_root, env=env, timeout=COMMAND_TIMEOUT_SECONDS,
                                started=started, phase="source compile " + source_rel)
                    if not object_path.is_file() or object_path.stat().st_size == 0:
                        raise BuildFailure("compiler reported success without a nonempty object: " + source_rel)
                    print(f"SOURCE_PASS path={source_rel} sha256={source_digest} object_bytes={object_path.stat().st_size}", flush=True)
                    component_objects.append(object_path)
                    object_manifest.append((name, object_name, object_path))

                if len(component_objects) != len(component_sources):
                    raise BuildFailure("component object count mismatch: " + name)
                all_objects[name] = component_objects
                print(f"COMPILE_PASS component={name} objects={len(component_objects)}/{len(component_sources)}", flush=True)

                archive = build_root / "archives" / ("lib" + name + ".a")
                run_checked([archiver, "rcs", str(archive), *(str(path) for path in component_objects)],
                            cwd=repo_root, env=env, timeout=COMMAND_TIMEOUT_SECONDS,
                            started=started, phase="archive " + name)
                if not archive.is_file() or archive.stat().st_size == 0:
                    raise BuildFailure("archiver reported success without a nonempty archive: " + name)
                members_result = run_checked([archiver, "t", str(archive)], cwd=repo_root, env=env,
                                             timeout=COMMAND_TIMEOUT_SECONDS, started=started,
                                             phase="archive member verification " + name)
                members = tuple(line for line in members_result.stdout.splitlines() if line)
                expected_members = tuple(path.name for path in component_objects)
                if members != expected_members:
                    raise BuildFailure("archive members do not exactly match component objects: " + name)
                archives.append(archive)
                print(f"ARCHIVE_PASS component={name} members={len(members)} bytes={archive.stat().st_size}", flush=True)

            if sum(map(len, all_objects.values())) != 20 or len(archives) != 6:
                raise BuildFailure("final object/archive count differs from 20/6")

            link_map = build_root / "portable-all-objects.map"
            linked = build_root / "portable-all-objects.so"
            link_command = [compiler, "-shared", "-Wl,--no-undefined", "-Wl,--whole-archive",
                            *(str(path) for path in archives), "-Wl,--no-whole-archive",
                            "-Wl,-Map=" + str(link_map), "-o", str(linked)]
            run_checked(link_command, cwd=repo_root, env=env, timeout=LINK_TIMEOUT_SECONDS,
                        started=started, phase="whole-object undefined-symbol link")
            if not linked.is_file() or linked.stat().st_size == 0 or not link_map.is_file():
                raise BuildFailure("linker succeeded without a nonempty output and map")
            try:
                map_text = link_map.read_text(encoding="utf-8", errors="replace")
            except OSError as exc:
                raise BuildFailure(f"cannot read linker map: {exc}") from None
            for component, object_name, _ in object_manifest:
                member = f"lib{component}.a({object_name})"
                if member not in map_text:
                    raise BuildFailure("linker map does not prove object inclusion: " + member)
                print("LINK_OBJECT_INCLUDED=" + member, flush=True)

            external_consumer_started = True
            validate_external_consumer(repo_root, build_root, archives, compiler, env, started)

            print("SOURCE_COUNT=20", flush=True)
            print("OBJECT_COUNT=20", flush=True)
            print("ARCHIVE_COUNT=6", flush=True)
            print("LINK_MAP_OBJECTS=20/20", flush=True)
            print("FINAL_LINK=PASS --no-undefined whole-archive", flush=True)
            print("LINK_ARTIFACT_NOT_EXECUTED=true", flush=True)
            print("PORTABLE_BUILD_LINK_RESULT=PASS", flush=True)
        return 0
    except BuildFailure as exc:
        print("PORTABLE_BUILD_LINK_RESULT=FAIL", file=sys.stderr, flush=True)
        print("REASON=" + str(exc), file=sys.stderr, flush=True)
        if not external_consumer_started:
            print("EXTERNAL_CONSUMER_RESULT=NOT_RUN", flush=True)
        return 1
    except (OSError, UnicodeError, ValueError, KeyError, TypeError) as exc:
        print("PORTABLE_BUILD_LINK_RESULT=FAIL", file=sys.stderr, flush=True)
        print("REASON=unexpected validation error: " + str(exc), file=sys.stderr, flush=True)
        if not external_consumer_started:
            print("EXTERNAL_CONSUMER_RESULT=NOT_RUN", flush=True)
        return 1


if __name__ == "__main__":
    sys.exit(main())
