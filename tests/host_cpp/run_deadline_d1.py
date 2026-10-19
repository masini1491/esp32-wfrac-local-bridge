"""Retained D1 + D2-P0/P1-T OFFLINE TEST-ONLY runner; no network adapter execution.

Parser acquisition is an explicit, read-only SDK-source operation, not product
network validation. Upstream files live only in TemporaryDirectory, not Git.
Local --sdk input is checked against canonical Git blob identities (CRLF-safe).
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import urllib.request


SDK_REVISION = "7101770dc6db2667b3c477cc31365dd1acd6db4e"
PARSER_BLOBS = {
    "http_parser.c": "ef1ecd48c6aec122a1dbe132096fcda9713b964c",
    "http_parser.h": "105ae510a8ab57509de2c68bbe1504e39ffb165e",
}


def checked_parser(data, expected):
    # SDK checkout on Windows may have autocrlf; canonical source is LF.
    canonical = data.replace(b"\r\n", b"\n")
    blob = b"blob " + str(len(canonical)).encode("ascii") + b"\0" + canonical
    actual = hashlib.sha1(blob).hexdigest()
    if actual != expected:
        raise ValueError(f"Pinned parser identity mismatch: {actual} != {expected}")
    return canonical


def run(command, cwd):
    print("COMMAND:", subprocess.list2cmdline([str(x) for x in command]), flush=True)
    subprocess.run(command, cwd=cwd, check=True, timeout=60)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--sdk", type=Path)
    source.add_argument("--fetch-pinned-parser", action="store_true")
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--cxx", default="g++")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    print(f"TESTED CHECKOUT SHA: {revision}", flush=True)
    for relative in ("tests/host_cpp/test_deadline_d1.cpp", "tests/host_cpp/test_deadline_d2_p0.cpp",
                     "tests/host_cpp/test_deadline_d2_p1.cpp",
                     "tests/host_cpp/run_deadline_d1.py"):
        content = (root / relative).read_bytes().replace(b"\r\n", b"\n")
        print(f"INPUT SHA256 {relative} {hashlib.sha256(content).hexdigest()}", flush=True)
    for compiler in (args.cc, args.cxx):
        if shutil.which(compiler) is None:
            raise RuntimeError(f"TOOLCHAIN prerequisite missing: {compiler}")
        run([compiler, "--version"], root)
    with tempfile.TemporaryDirectory(prefix="wfrac-d1-") as temp:
        build = Path(temp)
        for name, identity in PARSER_BLOBS.items():
            if args.sdk:
                data = (args.sdk / "components" / "http_parser" / name).read_bytes()
            else:
                url = ("https://raw.githubusercontent.com/espressif/esp-idf/"
                       f"{SDK_REVISION}/components/http_parser/{name}")
                with urllib.request.urlopen(url, timeout=30) as response:
                    data = response.read(256 * 1024 + 1)
                if len(data) > 256 * 1024:
                    raise ValueError("SDK parser download exceeds bounded acquisition size")
            (build / name).write_bytes(checked_parser(data, identity))
            print(f"PINNED SDK {SDK_REVISION} {name} git-blob={identity}", flush=True)
        obj = build / "http_parser.o"
        exe = build / ("deadline-d1.exe" if os.name == "nt" else "deadline-d1")
        # Compile the unmodified upstream parser as C, not an imitation or C++ port.
        run([args.cc, "-std=c99", "-Wall", "-I", str(build), "-c",
             str(build / "http_parser.c"), "-o", str(obj)], root)
        run([args.cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
             "-I", str(build), "-I", str(root / "components/wfrac_transport/include"),
             str(root / "tests/host_cpp/test_deadline_d2_p1.cpp"),
             str(root / "components/wfrac_transport/src/scheme_policy.cpp"),
             str(obj), "-o", str(exe)], root)
        run([str(exe)], root)


if __name__ == "__main__":
    main()
