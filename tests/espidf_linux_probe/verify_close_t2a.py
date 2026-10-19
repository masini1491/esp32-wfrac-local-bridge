"""CLOSE-T2A: pinned native Linux SDK evidence, plus synthetic verifier guards.

Run only inside independently established no-egress isolation. A process exit is
test termination, never worker/PCB reclamation. Existing CI runs default ELF;
this local verifier additionally runs the missing-release negative separately.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import unittest

from verify_close_t1 import require_config, require_trace as require_t1


PINS = {".": "7101770dc6db2667b3c477cc31365dd1acd6db4e",
        "components/lwip/lwip": "fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e"}
CONFIG = "T2A CONFIG LWIP_SOCKET=1 LWIP_IPV4=1 LWIP_HAVE_LOOPIF=0 LWIP_NETIF_LOOPBACK=0 no_netif=1"
ORDER = (CONFIG, "T2A PASS 1:", "T2A TRACE socket_created fd=",
         "T2A TRACE core_held", "T2A TRACE pending posts=",
         "T2A TRACE explicit_release", "T2A TRACE close_returned result=0 owner=retained",
         "T2A TRACE owner_invalidated after_close_return", "T2A PASS 2:",
         "T2A SDK_RUNTIME_CONFIRMED: 2/2 never-connected CLOSED BSD socket cases only")


def require_trace(text, negative=False):
    require_t1(text.split("T2A CONFIG", 1)[0])
    if CONFIG not in text or "FAIL:" in text:
        raise ValueError("unsafe or failed T2A trace")
    markers = ORDER[:5] if negative else ORDER
    positions = [text.find(m) for m in markers]
    if (any(p < 0 for p in positions) or positions != sorted(set(positions))
            or any(text.count(m) != 1 for m in markers)):
        raise ValueError("missing/reordered T2A native trace")
    pending = re.search(r"T2A TRACE pending posts=([1-9][0-9]*) waits=([1-9][0-9]*) fd=([0-9]+) owner=retained no_netif=1", text)
    created = re.search(r"T2A TRACE socket_created fd=([0-9]+) owner=retained no_netif=1", text)
    if pending is None or created is None or pending[3] != created[1]:
        raise ValueError("missing actual post/wait and retained same descriptor")
    if negative:
        if ("T2A TEST_TERMINATION: missing release; retained owner; no cleanup claim" not in text
                or any(m in text for m in ORDER[5:])):
            raise ValueError("negative termination cannot claim release/completion")
    elif "TEST_TERMINATION:" in text:
        raise ValueError("termination is not runtime completion")


def require_calls(disassembly, callees):
    for symbol in callees:
        if not re.search(r"\b(?:call\w*|jmp\w*)\s+[^\n]*<" + re.escape(symbol) + r">", disassembly):
            raise ValueError("missing genuine SDK call/observer routing: " + symbol)


def command(*args):
    return subprocess.check_output(args, text=True, timeout=15)


def proof(build):
    require_config((build / "sdkconfig").read_text())
    binary = build / "wfrac_a6b_linux.elf"
    if "Advanced Micro Devices X86-64" not in command("readelf", "-h", str(binary)):
        raise ValueError("not the admitted native Linux x86-64 ELF")
    sdk = Path(os.environ["IDF_PATH"]).resolve()
    for path, pin in PINS.items():
        if command("git", "-C", str(sdk / path), "rev-parse", "HEAD").strip() != pin:
            raise ValueError("SDK pin mismatch: " + path)
    lwip = sdk / "components/lwip/lwip"
    source = lwip / "src/api/sockets.c"
    blob = command("git", "-C", str(lwip), "rev-parse", "HEAD:src/api/sockets.c").strip()
    if command("git", "-C", str(lwip), "hash-object", str(source)).strip() != blob:
        raise ValueError("SDK sockets.c bytes differ from pinned object")
    inputs = json.loads((build / "compile_commands.json").read_text())
    if not any(Path(c["file"]).resolve() == source.resolve() for c in inputs):
        raise ValueError("actual pinned BSD socket source not compiled")
    symbols = command("nm", str(binary))
    for name in ("lwip_socket", "lwip_close", "lwip_fcntl", "netconn_prepare_delete",
                 "netconn_delete", "free_socket", "tcpip_send_msg_wait_sem",
                 "__wrap_sys_mbox_post", "sys_mbox_post",
                 "__wrap_sys_arch_sem_wait", "sys_arch_sem_wait"):
        if re.search(r" [Tt] " + name + r"$", symbols, re.M) is None:
            raise ValueError("missing genuine SDK symbol: " + name)
    def code(name):
        return command("objdump", "-d", "--disassemble=" + name, str(binary))
    require_calls(code("lwip_close"), ("netconn_prepare_delete", "free_socket"))
    free_code = code("free_socket")
    if "<free_socket_free_elements>" in free_code:
        free_code += code("free_socket_free_elements")
    require_calls(free_code, ("netconn_delete",))
    require_calls(code("tcpip_send_msg_wait_sem"),
                  ("__wrap_sys_mbox_post", "__wrap_sys_arch_sem_wait"))
    require_calls(code("__wrap_sys_mbox_post"), ("sys_mbox_post",))
    require_calls(code("__wrap_sys_arch_sem_wait"), ("sys_arch_sem_wait",))
    print("T2A ELF PROOF " + json.dumps({"pins": PINS, "socket_source": str(source),
          "socket_blob": blob, "elf_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
          "actual_close_and_observer_routing": "PASS"}, sort_keys=True), flush=True)
    return binary


def run(build):
    binary = proof(build)
    for negative in (False, True):
        env = os.environ.copy()
        env.pop("WFRAC_T1_NEGATIVE", None)
        env.pop("WFRAC_T2A_NEGATIVE", None)
        if negative:
            env["WFRAC_T2A_NEGATIVE"] = "no-release"
        try:
            result = subprocess.run([str(binary)], env=env, capture_output=True,
                                    text=True, timeout=20, check=False)
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError("TEST_TERMINATION: watchdog; cleanup UNPROVEN") from exc
        print(result.stdout, end="", flush=True)
        if result.returncode != (2 if negative else 0):
            raise RuntimeError("unexpected native test result: " + str(result.returncode) + result.stderr)
        require_trace(result.stdout, negative)
    print("T2A negative PASS: TEST_TERMINATION with retained owner, not cleanup", flush=True)


class VerifierTests(unittest.TestCase):
    """Synthetic trace/config/route rejection, NOT native SDK evidence."""
    def good(self):
        from verify_close_t1 import REQUIRED
        lines = list(ORDER)
        lines[2] += "54 owner=retained no_netif=1"
        lines[4] += "1 waits=1 fd=54 owner=retained no_netif=1"
        return "\n".join(REQUIRED + (
            "T1 UNPROVEN: process exit is TEST_TERMINATION not SDK shutdown",
        ) + tuple(lines))

    def test_positive_control(self):
        require_trace(self.good())

    def test_every_missing_marker(self):
        for marker in ORDER:
            with self.subTest(marker=marker), self.assertRaises(ValueError):
                require_trace(self.good().replace(marker, "MISSING"))

    def test_reordered_return_release(self):
        text = self.good().replace(ORDER[5], "SWAP").replace(ORDER[6], ORDER[5]).replace("SWAP", ORDER[6])
        with self.assertRaises(ValueError):
            require_trace(text)

    def test_owner_cleared_before_return(self):
        text = self.good().replace(ORDER[6], "SWAP").replace(ORDER[7], ORDER[6]).replace("SWAP", ORDER[7])
        with self.assertRaises(ValueError):
            require_trace(text)

    def test_no_actual_wait(self):
        with self.assertRaises(ValueError):
            require_trace(self.good().replace("waits=1", "waits=0"))

    def test_changed_owner_descriptor(self):
        with self.assertRaises(ValueError):
            require_trace(self.good().replace("waits=1 fd=54", "waits=1 fd=55"))

    def test_termination_cannot_pass(self):
        with self.assertRaises(ValueError):
            require_trace(self.good() + "\nT2A TEST_TERMINATION: no release")

    def test_negative_retains_owner_without_completion(self):
        text = self.good().split(ORDER[5])[0]
        text += "T2A TEST_TERMINATION: missing release; retained owner; no cleanup claim"
        require_trace(text, negative=True)
        with self.assertRaises(ValueError):
            require_trace(text + "\n" + ORDER[7], negative=True)

    def test_duplicate_marker(self):
        with self.assertRaises(ValueError):
            require_trace(self.good() + "\n" + ORDER[7])

    def test_wrong_runtime_config(self):
        with self.assertRaises(ValueError):
            require_trace(self.good().replace("LWIP_SOCKET=1", "LWIP_SOCKET=0"))

    def test_unsafe_generated_config(self):
        with self.assertRaises(ValueError):
            require_config("CONFIG_IDF_TARGET_LINUX=y\nCONFIG_LWIP_ENABLE=y\nCONFIG_LWIP_NETIF_LOOPBACK=y")

    def test_wrong_elf_route(self):
        with self.assertRaises(ValueError):
            require_calls("call 123 <fake_post>", ("sys_mbox_post",))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("build", type=Path, nargs="?")
    args = parser.parse_args()
    result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(VerifierTests))
    if not result.wasSuccessful():
        raise SystemExit(1)
    print(f"T2A verifier: {result.testsRun}/{result.testsRun} SYNTHETIC_PASS (missing-marker subcases also checked)", flush=True)
    if args.build:
        run(args.build.resolve())
