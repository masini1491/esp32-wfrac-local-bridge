"""Static CLOSE-T3-FEAS checks only: NEVER load/execute the linked fixture ELF."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import unittest


PINS = {".": "7101770dc6db2667b3c477cc31365dd1acd6db4e",
        "components/lwip/lwip": "fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e"}
REFERENCES = ("netif_init", "netif_loop_output", "netif_poll")


def require_config(text):
    lines = set(text.splitlines())
    for expected in ('CONFIG_IDF_TARGET="linux"', "CONFIG_IDF_TARGET_LINUX=y",
                     "CONFIG_LWIP_ENABLE=y", "CONFIG_LWIP_IPV4=y",
                     "CONFIG_LWIP_NETIF_LOOPBACK=y"):
        if expected not in lines:
            raise ValueError("wrong/missing isolated Linux config: " + expected)


def require_macros(text):
    for name in ("LWIP_HAVE_LOOPIF", "LWIP_NETIF_LOOPBACK"):
        if re.search(r"^#define " + name + r" 1$", text, re.M) is None:
            raise ValueError("wrong actual preprocessor mapping: " + name)


def require_anchor(code):
    if re.search(r"\b(?:call\w*|jmp\w*)\s+", code):
        raise ValueError("anchor must contain no runtime function calls or tail jumps")
    for name in REFERENCES:
        if "<" + name + ">" not in code:
            raise ValueError("anchor does not retain real function address: " + name)


def require_isolation(info):
    host = info["HostConfig"]
    if (host["NetworkMode"] != "none" or not host["ReadonlyRootfs"]
            or host.get("Privileged") or "ALL" not in host["CapDrop"]
            or "no-new-privileges" not in host["SecurityOpt"]
            or host.get("PortBindings") or host.get("CapAdd")):
        raise ValueError("container isolation configuration is insufficient")
    mounts = {m["Destination"]: m for m in info["Mounts"]}
    if (set(mounts) != {"/src", "/out"} or mounts["/src"]["RW"] is not False
            or mounts["/out"]["RW"] is not True):
        raise ValueError("only read-only source / separate writable output mounts admitted")


def command(*args, cwd=None):
    return subprocess.check_output(args, cwd=cwd, text=True, timeout=20)


def verify(build):
    config = (build / "sdkconfig").read_text()
    require_config(config)
    sdk = Path(os.environ["IDF_PATH"]).resolve()
    for path, pin in PINS.items():
        if command("git", "-C", str(sdk / path), "rev-parse", "HEAD").strip() != pin:
            raise ValueError("pinned SDK mismatch: " + path)
    sources = ((sdk, "components/lwip/Kconfig"),
               (sdk, "components/lwip/port/include/lwipopts.h"),
               (sdk / "components/lwip/lwip", "src/core/netif.c"))
    blobs = {}
    for repo, path in sources:
        expected = command("git", "-C", str(repo), "rev-parse", "HEAD:" + path).strip()
        if command("git", "-C", str(repo), "hash-object", str(repo / path)).strip() != expected:
            raise ValueError("SDK working source differs from pinned blob: " + path)
        blobs[str(repo / path)] = expected
    entries = json.loads((build / "compile_commands.json").read_text())
    netif_source = (sdk / "components/lwip/lwip/src/core/netif.c").resolve()
    if not any(Path(e["file"]).resolve() == netif_source for e in entries):
        raise ValueError("actual pinned netif.c source was not compiled")
    anchors = [e for e in entries if Path(e["file"]).name == "loopback_link.cpp"]
    if len(anchors) != 1:
        raise ValueError("ambiguous/missing isolated compile anchor")
    entry = anchors[0]
    args = shlex.split(entry["command"])
    output_at = args.index("-o")
    obj = (Path(entry["directory"]) / args[output_at + 1]).resolve()
    del args[output_at:output_at + 2]
    args.remove("-c")
    macros = subprocess.check_output(args + ["-E", "-dM"], cwd=entry["directory"],
                                     text=True, timeout=20)
    require_macros(macros)
    relocations = command("objdump", "-r", str(obj))
    for name in REFERENCES:
        if re.search(r"\b" + name + r"(?:[+-]|\s|$)", relocations) is None:
            raise ValueError("missing anchor object relocation: " + name)
    binary = build / "wfrac_t3_feas_link_only.elf"
    if "Advanced Micro Devices X86-64" not in command("readelf", "-h", str(binary)):
        raise ValueError("not the actual Linux x86-64 ELF")
    symbols = command("nm", str(binary))
    for name in (*REFERENCES, "netif_loopif_init", "netif_loop_output_ipv4", "loop_netif"):
        if re.search(r" [TtBbDd] " + name + r"$", symbols, re.M) is None:
            raise ValueError("missing retained actual SDK loopback symbol: " + name)
    anchor = command("objdump", "-d", "--disassemble=app_main", str(binary))
    require_anchor(anchor)
    netif_code = command("objdump", "-d", "--disassemble=netif_init", str(binary))
    for name in ("netif_add", "loop_netif", "netif_loopif_init"):
        if "<" + name not in netif_code:
            raise ValueError("actual loopback init link path missing: " + name)
    link_map = (build / "wfrac_t3_feas_link_only.map").read_text()
    cross_ref = link_map.split("Cross Reference Table", 1)
    if len(cross_ref) != 2:
        raise ValueError("missing actual linker cross-reference table")
    for name in REFERENCES:
        section = re.search(r"^" + name + r"\s+.*(?:\n[ \t]+.*)*", cross_ref[1], re.M)
        if section is None or "loopback_link.cpp" not in section[0] or "liblwip.a(netif.c" not in section[0]:
            raise ValueError("loopback producer/anchor consumer map missing: " + name)
    print("T3_FEAS_PROOF " + json.dumps({"tier": "SDK_COMPILE_ONLY", "pins": PINS,
          "source_blobs": blobs, "config_sha256": hashlib.sha256(config.encode()).hexdigest(),
          "LWIP_HAVE_LOOPIF": 1, "LWIP_NETIF_LOOPBACK": 1,
          "elf_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
          "actual_object_relocations_and_map": "PASS", "anchor_runtime_calls": "NONE",
          "fixture_execution": "NOT_RUN"}, sort_keys=True), flush=True)


class GuardTests(unittest.TestCase):
    def config(self):
        return '\n'.join(('CONFIG_IDF_TARGET="linux"', "CONFIG_IDF_TARGET_LINUX=y",
                          "CONFIG_LWIP_ENABLE=y", "CONFIG_LWIP_IPV4=y", "CONFIG_LWIP_NETIF_LOOPBACK=y"))

    def test_config_positive(self):
        require_config(self.config())

    def test_loopback_disabled_rejected(self):
        with self.assertRaises(ValueError):
            require_config(self.config().replace("CONFIG_LWIP_NETIF_LOOPBACK=y", "# disabled"))

    def test_wrong_target_rejected(self):
        with self.assertRaises(ValueError):
            require_config(self.config().replace('CONFIG_IDF_TARGET="linux"', 'CONFIG_IDF_TARGET="esp32c3"'))

    def test_macro_mapping(self):
        require_macros("#define LWIP_HAVE_LOOPIF 1\n#define LWIP_NETIF_LOOPBACK 1")
        with self.assertRaises(ValueError):
            require_macros("#define LWIP_HAVE_LOOPIF 0\n#define LWIP_NETIF_LOOPBACK 1")

    def test_no_call_anchor(self):
        require_anchor("\n".join("lea <" + n + ">" for n in REFERENCES))

    def test_runtime_call_rejected(self):
        with self.assertRaises(ValueError):
            require_anchor("call 123 <netif_init>")

    def test_missing_reference_rejected(self):
        with self.assertRaises(ValueError):
            require_anchor("lea <netif_init>")

    def isolation(self):
        return {"HostConfig": {"NetworkMode": "none", "ReadonlyRootfs": True,
                "Privileged": False, "CapDrop": ["ALL"], "CapAdd": None,
                "SecurityOpt": ["no-new-privileges"], "PortBindings": {}},
                "Mounts": [{"Destination": "/src", "RW": False}, {"Destination": "/out", "RW": True}]}

    def test_isolation_positive(self):
        require_isolation(self.isolation())

    def test_external_network_rejected(self):
        info = self.isolation()
        info["HostConfig"]["NetworkMode"] = "bridge"
        with self.assertRaises(ValueError):
            require_isolation(info)

    def test_writable_source_rejected(self):
        info = self.isolation()
        info["Mounts"][0]["RW"] = True
        with self.assertRaises(ValueError):
            require_isolation(info)

    def test_sdk_overlay_rejected(self):
        info = self.isolation()
        info["Mounts"].append({"Destination": "/opt/esp/idf", "RW": True})
        with self.assertRaises(ValueError):
            require_isolation(info)

    def test_privileged_container_rejected(self):
        info = self.isolation()
        info["HostConfig"]["Privileged"] = True
        with self.assertRaises(ValueError):
            require_isolation(info)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("build", nargs="?", type=Path)
    parser.add_argument("--inspect", type=Path)
    args = parser.parse_args()
    result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(GuardTests))
    if not result.wasSuccessful():
        raise SystemExit(1)
    print(f"T3_FEAS_GUARDS {result.testsRun}/{result.testsRun} SYNTHETIC_PASS", flush=True)
    if args.inspect:
        data = json.loads(args.inspect.read_text())
        require_isolation(data[0] if isinstance(data, list) else data)
        print("T3_FEAS_ISOLATION configured-only PASS; no packet probe or escape-proof claim", flush=True)
    if args.build:
        verify(args.build.resolve())
