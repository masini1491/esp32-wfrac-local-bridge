"""Explicit, offline local workflow consumer. Never installs, pulls, fetches or publishes.

Requires existing PyYAML. Workflow hashes deliberately fail on drift: independent
review must approve any new command/condition/matrix/check contract before repinning.
Local results are NOT a replacement for required GitHub checks.
"""
import argparse
import hashlib
import importlib.util
import itertools
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import tempfile
import threading
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]


class Stop(RuntimeError):
    pass


class InteractiveAuthRequired(Stop):
    pass


class OperationTimeout(Stop):
    pass


AUTH_HELP = ("INTERACTIVE_AUTH_REQUIRED; Docker suites NOT_RUN. In your OWN terminal, "
             "run only the separately approved command: sudo docker image inspect "
             "espressif/idf:v6.0.2 --format '{{json .RepoDigests}} {{.Id}} {{.Os}}/{{.Architecture}}'. "
             "Never send the password to Codex. Re-run preflight in the SAME authorized terminal; "
             "a ticket in another terminal may not apply.")


def noninteractive(args):
    executable = Path(args[0]).name.lower().removesuffix(".exe")
    if executable == "sudo":
        if args[1:3] != ["-n", "docker"]:
            raise InteractiveAuthRequired(AUTH_HELP)
        # Only the explicitly approved Docker route, never sudo shells or login.
        noninteractive(args[2:])
    if executable in ("su", "login", "runas"):
        raise InteractiveAuthRequired(AUTH_HELP)
    if ((executable == "gh" and args[1:3] == ["auth", "login"]) or
            (executable == "docker" and args[1:2] == ["login"])):
        raise InteractiveAuthRequired("INTERACTIVE_AUTH_REQUIRED: login requires separate human approval")
    if executable in ("bash", "sh") and any(a in ("-c", "-ec", "-lc") for a in args):
        index = next(i for i, a in enumerate(args) if a in ("-c", "-ec", "-lc"))
        script = args[index + 1]
        if re.search(r"\b(?:sudo|su|login)\b|\bgh\s+auth\s+login\b", script):
            raise InteractiveAuthRequired(AUTH_HELP)


SENSITIVE_MARKER = re.compile(
    r"(?i)\b(?:authorization|proxy[-_ ]?authorization|password|passwd|passphrase|"
    r"(?:access[-_ ]?|refresh[-_ ]?)?token|operator[-_ ]?id|(?:client[-_ ]?)?secret|"
    r"credentials?|private[-_ ]?key|wifi[-_ ]?(?:password|credentials?))\b[\"']?\s*[:=]"
    r"|-----BEGIN [^\r\n]*PRIVATE KEY-----")


def redact(text):
    # A value may continue across blank/unindented lines or JSON/YAML containers.
    # Never guess its end; fail closed for the rest of this diagnostic/log stream.
    if SENSITIVE_MARKER.search(text):
        return "[REDACTED SENSITIVE LOG TAIL]\n"
    return re.sub(r"https://[^\s/@]+:[^\s/@]+@", "https://[REDACTED]@", text)


def command(args, *, cwd=None, timeout=30, limit=1048576):
    """Drain continuously, retain bounded sanitized lines only; no raw on-disk log."""
    noninteractive(args)
    process = subprocess.Popen(args, cwd=cwd, stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    retained, used = [], 0
    def drain():
        nonlocal used
        pending, dropping, suppressed = b"", False, False
        while True:
            chunk = process.stdout.read(4096)
            if not chunk:
                chunk = b"\n"
                ended = True
            else:
                ended = False
            for part in chunk.splitlines(keepends=True):
                pending += part
                if len(pending) > 8192:
                    dropping = True
                    pending = pending[-1:]
                if part.endswith(b"\n"):
                    line = pending.decode("utf-8", errors="replace")
                    if not suppressed and used < limit:
                        safe = ("[REDACTED UNCLASSIFIABLE LOG TAIL]\n" if dropping
                                else redact(line)).encode("utf-8")[:limit-used]
                        retained.append(safe)
                        used += len(safe)
                    suppressed = suppressed or dropping or bool(SENSITIVE_MARKER.search(line))
                    pending, dropping = b"", False
            if ended:
                break
    reader = threading.Thread(target=drain, daemon=True)
    reader.start()
    try:
        result = process.wait(timeout=timeout)
    except (subprocess.TimeoutExpired, KeyboardInterrupt):
        process.kill()
        process.wait(timeout=10)
        raise OperationTimeout("operation timeout/interrupted; no PASS") from None
    finally:
        reader.join(timeout=5)
        process.stdout.close()
    if reader.is_alive():
        raise Stop("output reader did not quiesce")
    output = b"".join(retained).decode("utf-8", errors="replace")
    if result and Path(args[0]).name == "sudo" and re.search(r"password.*required|terminal.*required|no tty", output, re.I):
        raise InteractiveAuthRequired(AUTH_HELP)
    return result, output


def checked(args, **kwargs):
    code, output = command(args, **kwargs)
    if code:
        if Path(args[0]).name == "docker" and "permission denied" in output.lower():
            raise Stop("DOCKER_ACCESS_REQUIRED; Docker suites NOT_RUN; no automatic sudo/group/socket change")
        raise Stop(redact(output.strip()) or "command failed without diagnostic")
    return output.strip()


def git(root, *args):
    return checked(["git", "-C", str(root), *args])


def identity(root, branch, sha=None):
    top = Path(git(root, "rev-parse", "--show-toplevel")).resolve()
    if top != root.resolve():
        raise Stop("repository root mismatch")
    origin = git(root, "remote", "get-url", "origin")
    if origin not in ("https://github.com/masini1491/esp32-wfrac-local-bridge.git",
                      "https://github.com/masini1491/esp32-wfrac-local-bridge",
                      "git@github.com:masini1491/esp32-wfrac-local-bridge.git"):
        raise Stop("repository origin mismatch (URL withheld)")
    head = git(root, "rev-parse", "HEAD")
    if git(root, "branch", "--show-current") != branch or (sha and head != sha):
        raise Stop("branch/HEAD identity mismatch")
    if git(root, "status", "--porcelain=v1", "--untracked-files=all"):
        raise Stop("dirty tree: preserve work; no stash/reset/clean")
    for state in ("MERGE_HEAD", "CHERRY_PICK_HEAD", "REVERT_HEAD", "rebase-merge", "rebase-apply"):
        path = Path(git(root, "rev-parse", "--git-path", state))
        if (path if path.is_absolute() else root / path).exists():
            raise Stop("unfinished Git operation")
    return {"sha": head, "tree": git(root, "rev-parse", "HEAD^{tree}")}


def blob(data):
    data = data.replace(b"\r\n", b"\n")
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def job_budget(job, local_cap=None):
    workflow = int(job["timeout-minutes"])*60 if "timeout-minutes" in job else None
    limits = [value for value in (workflow, local_cap) if value is not None]
    if not limits or any(value <= 0 for value in limits):
        raise Stop("invalid/unresolved job timeout budget")
    return {"timeout": min(limits), "workflow_timeout_seconds": workflow,
            "local_timeout_cap_seconds": local_cap}


def sdk_routing(job, key):
    # Local profiles select explicitly offline; only this reviewed remote guard is
    # understood, not ignored or generalized to arbitrary Actions expressions.
    expected = "${{ needs.scope.outputs." + key + " == 'true' }}"
    if job.get("needs") != "scope" or job.get("if") != expected:
        raise Stop("unsupported SDK needs/if selection contract")


def contract(root):
    config = json.loads((root / "tools/local-ci.config.json").read_text())
    try:
        import yaml
    except ImportError:
        raise Stop("PyYAML prerequisite missing; request approved installation, never auto-install") from None
    class WorkflowLoader(yaml.BaseLoader):
        def construct_mapping(self, node, deep=False):
            keys = [self.construct_object(key, deep=deep) for key, _ in node.value]
            if any(not isinstance(key, str) for key in keys) or len(keys) != len(set(keys)):
                raise Stop("duplicate/unsupported workflow mapping key")
            return super().construct_mapping(node, deep=deep)
    documents = {}
    for path, expected in config["workflow_blobs"].items():
        raw = (root / path).read_bytes()
        if blob(raw) != expected:
            raise Stop("workflow drift: commands/conditions/matrix/required-check review needed: " + path)
        documents[path] = yaml.load(raw.decode(), Loader=WorkflowLoader)
    host = documents[".github/workflows/validate-contracts.yml"]["jobs"]["validate-contracts"]
    sdk = documents[".github/workflows/esp-idf-compile.yml"]["jobs"]
    if "if" in host or "needs" in host:
        raise Stop("Host must remain unconditional for PR and main push")
    for job, key in (("linux-dns", "dns"), ("linux-callback", "callback"), ("compile", "compile")):
        sdk_routing(sdk[job], key)
    jobs = {"host": {"job": host, "env": {}, **job_budget(host, config["host_timeout_seconds"])}}
    for name, key in (("a6b", "linux-callback"), ("a7b", "linux-dns")):
        jobs[name] = {"job": sdk[key], "env": {}, **job_budget(sdk[key])}
    matrix = sdk["compile"]["strategy"]["matrix"]
    keys = [key for key in matrix if key != "include"]
    cells = [dict(zip(keys, values)) for values in itertools.product(*(matrix[key] for key in keys))]
    cells.extend(matrix.get("include", []))
    for cell in cells:
        target, firmware, variant = cell["target"], cell["firmware"], cell["variant"]
        label = "smoke" if firmware == "firmware/idf_compile_smoke" else "probe"
        name = f"{target}-{label}-{variant}"
        if name in jobs:
            raise Stop("duplicate matrix cell")
        jobs[name] = {"job": sdk["compile"], "env": {"IDF_TARGET": target, "FIRMWARE": firmware,
                                                    "VARIANT": variant}, **job_budget(sdk["compile"])}
    if len(jobs) != 9:
        raise Stop("full profile must retain Host, A6b, A7b and SIX firmware/A5 cells")
    for name, value in jobs.items():
        value["steps"] = []
        for step in value["job"]["steps"]:
            if "uses" in step:
                if step["uses"] != "actions/checkout@v4":
                    raise Stop("unsupported workflow action")
                continue  # Source identity is checked independently; never remote checkout in tests.
            if "if" in step or "${{" in step.get("run", ""):
                raise Stop("unsupported workflow condition/expression")
            original = step["run"]
            effective = original
            if step["name"] == "Run D1 offline deadline contract tests (pinned SDK parser)":
                if original.count("--fetch-pinned-parser") != 1:
                    raise Stop("unverified D1 acquisition semantics")
                effective = original.replace("--fetch-pinned-parser", '--sdk "$IDF_PATH"')
            value["steps"].append({"name": step["name"], "command": effective,
                                   "workflow_command_sha256": hashlib.sha256(original.encode()).hexdigest()})
    return config, jobs


def select(jobs, profile=None, suite=None):
    if suite:
        if suite not in jobs:
            raise Stop("unknown suite/scope; no silent skip or full fallback")
        return [suite]
    if profile == "full":
        return list(jobs)
    if profile == "targeted":
        return ["host", "a6b", "a7b", "esp32c3-smoke-default", "esp32c6-smoke-default"]
    raise Stop("explicit targeted/full profile or exact suite required")


def docker_prefix(sudo):
    return ["sudo", "-n", "docker"] if sudo else ["docker"]


def prepare_suite_temp(root, output):
    """Fresh, private temp storage in this suite's writable /out bind, not /tmp."""
    temporary = output / "tmp"
    if output.resolve().is_relative_to(root.resolve()):
        raise Stop("suite temporary directory must be outside source tree")
    if output.is_symlink() or not output.is_dir() or temporary.exists() or temporary.is_symlink():
        raise Stop("suite temporary directory must be fresh output-only storage")
    temporary.mkdir(mode=0o700)
    return temporary


def image_info(prefix, config, digest):
    # Context inspection reads local client configuration, not a remote daemon.
    context = json.loads(checked(prefix + ["context", "inspect"]))[0]
    endpoint = os.environ.get("DOCKER_HOST") or context["Endpoints"]["docker"]["Host"]
    if not re.fullmatch(r"unix:///(?:var/run/docker\.sock|run/user/\d+/docker\.sock)", endpoint):
        raise Stop("non-local Docker endpoint rejected")
    info = json.loads(checked(prefix + ["image", "inspect", config["image"]]))[0]
    if info["Os"] + "/" + info["Architecture"] != config["platform"]:
        raise Stop("wrong image OS/architecture")
    if not digest or not re.fullmatch(r"sha256:[0-9a-f]{64}", digest):
        raise Stop("supply independently reviewed --image-digest; no floating tag trust")
    if "espressif/idf@" + digest not in info.get("RepoDigests", []):
        raise Stop("image digest mismatch; no pull/upgrade")
    if not re.fullmatch(r"sha256:[0-9a-f]{64}", info["Id"]):
        raise Stop("image content identity unavailable")
    return {"id": info["Id"], "digest": digest, "platform": config["platform"]}


def docker_args(prefix, image, root, output, name, inside, branch, sha):
    if output.resolve().is_relative_to(root.resolve()):
        raise Stop("output must be outside source tree")
    if any("," in str(path) or "\n" in str(path) for path in (root, output)):
        raise Stop("unsupported bind path")
    return prefix + ["run", "--rm", "--pull=never", "--name", name, "--network=none", "--read-only",
        "--cap-drop=ALL", "--security-opt=no-new-privileges", "--user", f"{os.getuid()}:{os.getgid()}",
        "--mount", f"type=bind,src={root},dst=/src,readonly",
        "--mount", f"type=bind,src={output},dst=/out", "--tmpfs", "/tmp:rw,nosuid,nodev,noexec",
        "-e", "TMPDIR=/out/tmp", "-e", "HOME=/out/home", "-e", "PYTHONPYCACHEPREFIX=/out/pycache",
        "-e", "WFRAC_OFFLINE=1", "-e", "GIT_OPTIONAL_LOCKS=0",
        "-e", "GIT_CONFIG_COUNT=3", "-e", "GIT_CONFIG_KEY_0=safe.directory", "-e", "GIT_CONFIG_VALUE_0=/src",
        "-e", "GIT_CONFIG_KEY_1=safe.directory", "-e", "GIT_CONFIG_VALUE_1=/opt/esp/idf",
        "-e", "GIT_CONFIG_KEY_2=safe.directory", "-e", "GIT_CONFIG_VALUE_2=/opt/esp/idf/components/lwip/lwip",
        "--entrypoint", "/bin/bash", image["id"], "-ec",
        '. "$IDF_PATH/export.sh" >/dev/null; exec python -B /src/tools/local_ci.py "$@"',
        "local-ci", "--repo", "/src", "--expected-branch", branch, "--expected-sha", sha,
        "--inside", inside]


def inside_check(config):
    if not Path("/.dockerenv").exists() or os.environ.get("WFRAC_OFFLINE") != "1":
        raise Stop("internal operation requires approved offline container")
    if set(p.name for p in Path("/sys/class/net").iterdir()) - {"lo"}:
        raise Stop("non-loopback network interface in offline container")
    sdk = Path(os.environ["IDF_PATH"])
    if checked(["idf.py", "--version"]) != config["idf_version"]:
        raise Stop("IDF version mismatch")
    if git(sdk, "rev-parse", "HEAD") != config["idf_sha"] or git(sdk / "components/lwip/lwip", "rev-parse", "HEAD") != config["lwip_sha"]:
        raise Stop("SDK/lwIP SHA mismatch")
    spec = importlib.util.spec_from_file_location("d1", ROOT / "tests/host_cpp/run_deadline_d1.py")
    d1 = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(d1)
    for file, expected in d1.PARSER_BLOBS.items():
        d1.checked_parser((sdk / "components/http_parser" / file).read_bytes(), expected)
    versions = {tool: checked([tool, "--version"]).splitlines()[0] for tool in ("python", "gcc", "g++", "cmake", "ninja")}
    Path("/out/provenance.json").write_text(json.dumps({"sdk": config["idf_sha"], "lwip": config["lwip_sha"],
                                                       "tools": versions}, indent=2))


def remaining_budget(deadline):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise OperationTimeout("workflow job timeout budget exhausted")
    return remaining


def run_steps(work, output, steps, deadline, log_bytes):
    """One shared monotonic job deadline; never give every step a fresh budget."""
    rows = []
    for step in steps:
        start = time.monotonic()
        code, log = command(["bash", "-eo", "pipefail", "-c", step["command"]], cwd=work,
                            timeout=remaining_budget(deadline), limit=log_bytes)
        (output / f"step-{len(rows):02d}.log").write_text(log)
        if code == 0:
            remaining_budget(deadline)  # A late zero exit is not job PASS.
        rows.append({**step, "status": "PASS" if code == 0 else "FAIL", "exit_code": code,
                     "wall_seconds": round(time.monotonic()-start, 6)})
        (output / "steps.json").write_text(json.dumps(rows, indent=2))
        if code:
            raise Stop("workflow step failed: " + step["name"])


def workspace_facade(root, work):
    """Writable output packages; original test modules remain read-only source links."""
    if work.resolve().is_relative_to(root.resolve()):
        raise Stop("workspace facade must be outside source tree")
    work.mkdir()
    for item in root.iterdir():
        if item.name not in (".git", "tests"):
            (work / item.name).symlink_to(item, target_is_directory=item.is_dir())

    def package(source, output, expand_host=False):
        output.mkdir()
        if not (source / "__init__.py").is_file():
            (output / "__init__.py").write_text("# Output-only local CI package; no source mutation.\n")
        for item in source.iterdir():
            if item.name == "__pycache__":
                continue
            if expand_host and item.name == "host" and item.is_dir():
                package(item, output / item.name)
            else:
                (output / item.name).symlink_to(item, target_is_directory=item.is_dir())

    # A regular installed 'tests' package otherwise shadows source namespace packages.
    package(root / "tests", work / "tests", expand_host=True)


def inside_job(root, config, jobs, name, identity_before, branch, sha):
    deadline = time.monotonic() + jobs.get(name, {}).get("timeout", 120)
    inside_check(config)
    remaining_budget(deadline)
    if name == "check":
        return
    work = Path("/out/work")
    workspace_facade(root, work)
    env = {"GITHUB_WORKSPACE": str(work), "GITHUB_SHA": identity_before["sha"], **jobs[name]["env"]}
    os.environ.update(env)
    run_steps(work, Path("/out"), jobs[name]["steps"], deadline, config["log_bytes"])
    if identity(root, branch, sha) != identity_before:
        raise Stop("source identity drift during validation")
    remaining_budget(deadline)


def sdk_evidence(report, name, directory, config):
    try:
        data = json.loads((directory / "provenance.json").read_text())
        if (not isinstance(data, dict) or data.get("sdk") != config["idf_sha"] or
                data.get("lwip") != config["lwip_sha"] or not isinstance(data.get("tools"), dict) or
                any(not isinstance(data["tools"].get(tool), str) or not data["tools"][tool]
                    for tool in ("python", "gcc", "g++", "cmake", "ninja"))):
            raise ValueError("invalid SDK provenance")
    except (OSError, ValueError, TypeError):
        report["sdk_integrity"][name] = {"status": "FAIL", "error": "required SDK evidence missing/invalid"}
        return False
    report["sdk_integrity"][name] = {"status": "PASS"}
    report.setdefault("provenance", {})[name] = data
    return True


def step_evidence(directory, expected):
    try:
        rows = json.loads((directory / "steps.json").read_text())
        if (not isinstance(rows, list) or any(not isinstance(row, dict) for row in rows) or
                [r.get("name") for r in rows] != [r["name"] for r in expected] or
                any(r.get("status") != "PASS" or r.get("exit_code") != 0 for r in rows)):
            raise ValueError("invalid steps")
    except (OSError, ValueError, TypeError):
        raise Stop("required steps.json evidence missing/invalid; attempted suite FAIL") from None
    return rows


def accounting(rows, selected, jobs):
    return (set(rows) == set(jobs) and all(rows[name]["status"] == "PASS" for name in selected)
            and all(rows[name]["status"] == "NOT_RUN" for name in set(jobs)-set(selected)))


def main(args):
    root = args.repo.resolve()
    before = identity(root, args.expected_branch, args.expected_sha)
    config, jobs = contract(root)
    if args.inside:
        inside_job(root, config, jobs, args.inside, before, args.expected_branch, args.expected_sha)
        return 0
    if args.plan:
        selected = select(jobs, args.profile, args.suite)
        print(json.dumps({**before, "selected": selected, "jobs": jobs, "runtime": "NOT_RUN"}, indent=2))
        return 0
    if Path(tempfile.gettempdir()).resolve().is_relative_to(root):
        raise Stop("temporary output root is inside source; choose an external TMPDIR")
    output = Path(tempfile.mkdtemp(prefix="wfrac-local-ci-"))
    report = {**before, "image": None, "profile": args.profile or args.suite or "check",
              "status": "FAIL", "full_profile": "NOT_RUN",
              "preflight": {"status": "NOT_RUN"}, "image_preflight": {"status": "NOT_RUN"},
              "sdk_integrity": {name: {"status": "NOT_RUN"} for name in jobs}, "cleanup": {},
              "job_budgets": {name: {key: job[key] for key in
                              ("timeout", "workflow_timeout_seconds", "local_timeout_cap_seconds")} for name, job in jobs.items()},
              "github_required_checks": "UNVERIFIED; not replaced by local results",
              "jobs": {name: {"status": "NOT_RUN"} for name in jobs}}
    (output / "result.json").write_text(json.dumps(report, indent=2))
    print("OUTPUT:", output, flush=True)
    try:
        report["preflight"] = {"status": "FAIL", "phase": "platform/scope"}
        if platform.system() != "Linux" or platform.machine() != "x86_64":
            raise Stop("use Linux x86_64 or PS7/WSL2 handoff; native Windows is NOT_RUN")
        selected = select(jobs, args.profile, args.suite) if args.run else []
        if args.run and not args.expected_sha:
            raise Stop("--run requires explicit --expected-sha")
        prefix = docker_prefix(args.sudo_docker)
        report["preflight"]["phase"] = "image"
        report["image_preflight"]["status"] = "FAIL"
        image = image_info(prefix, config, args.image_digest)
        report["image"] = image
        report["image_preflight"]["status"] = "PASS"
        report["preflight"]["status"] = "PASS"
        for name in selected or ["check"]:
            directory = output / name
            directory.mkdir()
            (directory / "home").mkdir()
            prepare_suite_temp(root, directory)  # Exists before SDK export or Python starts.
            container = "wfrac-ci-" + uuid.uuid4().hex
            argv = docker_args(prefix, image, root, directory, container, name, args.expected_branch, before["sha"])
            start = time.monotonic()
            deadline = start + jobs.get(name, {}).get("timeout", 120)
            report["sdk_integrity"].setdefault(name, {"status": "NOT_RUN"})
            try:
                code, log = command(argv, timeout=remaining_budget(deadline),
                                    limit=config["log_bytes"])
                remaining_budget(deadline)
            except InteractiveAuthRequired:
                # No second credential attempt or cleanup command after an auth gate.
                raise
            except Stop as primary:
                sdk_evidence(report, name, directory, config)
                if name != "check":
                    report["jobs"][name] = {"status": "FAIL", "error": redact(str(primary)),
                                            "wall_seconds": round(time.monotonic()-start, 6)}
                # Exact generated container name only; never clean unrelated containers/files.
                # Secondary cleanup failure must not mask the original cause or trigger retries.
                try:
                    checked(prefix + ["rm", "-f", container])
                    report["cleanup"][name] = {"status": "PASS"}
                except (Stop, OSError, ValueError) as secondary:
                    report["cleanup"][name] = {"status": "FAIL", "error": redact(str(secondary)),
                        "error_code": "INTERACTIVE_AUTH_REQUIRED" if isinstance(secondary, InteractiveAuthRequired)
                                      else "CLEANUP_FAILED"}
                raise
            # Once the container returned, evidence failure is FAIL, never NOT_RUN.
            if name != "check":
                report["jobs"][name] = {"status": "FAIL", "exit_code": code}
            (directory / "container.log").write_text(log)
            if not sdk_evidence(report, name, directory, config):
                raise Stop("required SDK integrity evidence missing/invalid; no suite PASS")
            if code:
                raise Stop("container failed; see sanitized log; remaining suites NOT_RUN")
            if name != "check":
                steps = step_evidence(directory, jobs[name]["steps"])
                report["jobs"][name] = {"status": "PASS", "steps": steps,
                                        "wall_seconds": round(time.monotonic()-start, 6)}
            if identity(root, args.expected_branch, before["sha"]) != before:
                raise Stop("source drift; no acceptance")
        report["status"] = "PASS" if not selected or accounting(report["jobs"], selected, jobs) else "FAIL"
        report["full_profile"] = "PASS" if len(selected) == 9 and report["status"] == "PASS" else "NOT_RUN"
    except (Stop, OSError, ValueError, KeyError) as error:
        report["status"], report["error"] = "FAIL", redact(str(error))
        report["error_code"] = ("INTERACTIVE_AUTH_REQUIRED" if isinstance(error, InteractiveAuthRequired)
                                else "OPERATION_TIMEOUT" if isinstance(error, OperationTimeout)
                                else "PREREQUISITE_OR_RUN_FAILURE")
        if args.profile == "full" and any(row["status"] == "FAIL" for row in report["jobs"].values()):
            report["full_profile"] = "FAIL"
    finally:
        (output / "result.json").write_text(json.dumps(report, indent=2))
        print(json.dumps(report, indent=2))
    return 0 if report.get("status") == "PASS" else 1


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=ROOT)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="default; no builds/downloads")
    mode.add_argument("--plan", action="store_true", help="workflow inspection only; no Docker")
    mode.add_argument("--run", action="store_true")
    profile = parser.add_mutually_exclusive_group()
    profile.add_argument("--profile", choices=("targeted", "full"))
    profile.add_argument("--suite")
    parser.add_argument("--expected-branch", default="main")
    parser.add_argument("--expected-sha")
    parser.add_argument("--image-digest", help="reviewed platform-specific sha256 digest, never a tag")
    parser.add_argument("--sudo-docker", action="store_true", help="explicit separate approval only; sudo -n")
    parser.add_argument("--inside", help=argparse.SUPPRESS)
    return parser.parse_args()


if __name__ == "__main__":
    ARGS = arguments()
    try:
        sys.exit(main(ARGS))
    except (Stop, OSError, ValueError, KeyError) as error:
        print("FAIL prerequisite; Docker suites NOT_RUN:", redact(str(error)), file=sys.stderr)
        sys.exit(1)
