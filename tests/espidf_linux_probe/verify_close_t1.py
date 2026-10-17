"""Local CLOSE-T1 proof/negative runner; real SDK evidence != synthetic guards.

Build first with pinned SDK and no egress, then pass the absolute build path.
The unchanged CI callback job runs the default binary (A6b 3 + T1 2 cases).
This runner additionally checks ELF routing and a fail-closed negative process.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import unittest


REQUIRED = (
    "A6B RUNTIME_CONFIRMED: 3/3",
    "T1 PASS 1:",
    "T1 TRACE: core held -> actual sys_mbox_post returned -> actual sem wait entered -> caller pending; owner retained",
    "T1 PASS 2:",
    "T1 SDK_RUNTIME_CONFIRMED: 2/2",
)


def require_config(text):
    lines = set(text.splitlines())
    for required in ("CONFIG_IDF_TARGET_LINUX=y", "CONFIG_LWIP_ENABLE=y",
                     "# CONFIG_LWIP_NETIF_LOOPBACK is not set",
                     "# CONFIG_LWIP_TCPIP_CORE_LOCKING is not set"):
        if required not in lines:
            raise ValueError("unsafe/unverified generated Linux config: " + required)


def require_trace(text):
    if any(marker not in text for marker in REQUIRED) or "FAIL:" in text or "TEST_TERMINATION:" in text:
        raise ValueError("incomplete/failed T1 SDK runtime evidence")
    if not text.index(REQUIRED[1]) < text.index(REQUIRED[2]) < text.index(REQUIRED[3]):
        raise ValueError("invalid T1 event order")


def run_binary(binary, negative=False):
    env = os.environ.copy()
    env.pop("WFRAC_T1_NEGATIVE", None)
    if negative:
        env["WFRAC_T1_NEGATIVE"] = "no-release"
    try:
        return subprocess.run([str(binary)], env=env, capture_output=True, text=True,
                              timeout=20, check=False)
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError("TEST_TERMINATION: external watchdog; quiescence UNPROVEN") from exc


def main(build):
    require_config((build / "sdkconfig").read_text())
    binary = build / "wfrac_a6b_linux.elf"
    symbols = subprocess.check_output(["nm", str(binary)], text=True)
    for symbol in ("tcpip_send_msg_wait_sem", "netconn_delete", "netconn_prepare_delete",
                   "lwip_netconn_do_delconn", "__wrap_sys_mbox_post", "sys_mbox_post",
                   "__wrap_sys_arch_sem_wait", "sys_arch_sem_wait"):
        if re.search(r" [Tt] " + symbol + r"$", symbols, re.M) is None:
            raise ValueError("missing actual SDK/observer symbol: " + symbol)
    disassembly = subprocess.check_output(
        ["objdump", "-d", "--disassemble=tcpip_send_msg_wait_sem", str(binary)], text=True)
    for symbol in ("__wrap_sys_mbox_post", "__wrap_sys_arch_sem_wait"):
        if not re.search(r"\bcall\w*\s+[^\n]*<" + symbol + r">", disassembly):
            raise ValueError("actual TCPIP API-message observer is not called: " + symbol)
    # Observer functions must delegate to the real SDK; admission/results remain real.
    for wrapper, original in (("__wrap_sys_mbox_post", "sys_mbox_post"),
                              ("__wrap_sys_arch_sem_wait", "sys_arch_sem_wait")):
        code = subprocess.check_output(["objdump", "-d", "--disassemble=" + wrapper,
                                        str(binary)], text=True)
        if not re.search(r"\bcall\w*\s+[^\n]*<" + original + r">", code):
            raise ValueError("observer does not delegate to actual SDK: " + wrapper)
    print("T1 ELF proof PASS: actual API-message path -> observers -> real post/sem wait", flush=True)
    result = run_binary(binary)
    print(result.stdout, end="", flush=True)
    if result.returncode != 0:
        raise RuntimeError("T1 binary failed: " + result.stderr)
    require_trace(result.stdout)
    negative = run_binary(binary, negative=True)
    print(negative.stdout, end="", flush=True)
    if (negative.returncode != 2 or "T1 TEST_TERMINATION: missing release; retained owner" not in negative.stdout
            or "T1 PASS 2:" in negative.stdout):
        raise ValueError("negative must terminate without completion/release claim")
    print("T1 negative PASS 1/1: missing release terminates isolated process, not quiescence", flush=True)


class VerifierTests(unittest.TestCase):
    # Synthetic verifier tests, NOT SDK runtime observations.
    def test_missing_marker(self):
        with self.assertRaises(ValueError):
            require_trace("\n".join(REQUIRED[:-1]))

    def test_termination_cannot_pass(self):
        with self.assertRaises(ValueError):
            require_trace("\n".join(REQUIRED) + "\nTEST_TERMINATION:")

    def test_reordered_events(self):
        with self.assertRaises(ValueError):
            require_trace("\n".join(reversed(REQUIRED)))

    def test_loopback_config_rejected(self):
        with self.assertRaises(ValueError):
            require_config("CONFIG_IDF_TARGET_LINUX=y\nCONFIG_LWIP_ENABLE=y\nCONFIG_LWIP_NETIF_LOOPBACK=y")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("build", type=Path, nargs="?")
    args = parser.parse_args()
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(VerifierTests)
    if not unittest.TextTestRunner().run(suite).wasSuccessful():
        raise SystemExit(1)
    if args.build:
        main(args.build.resolve())
