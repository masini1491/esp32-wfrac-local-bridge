"""Fail-closed A7b ELF interception proof and bounded offline SDK execution."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

def run(args, **kwargs):
    return subprocess.run(args, check=True, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=60, **kwargs).stdout

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    build = args.build.resolve()
    elf = build / "wfrac_a7b_dns.elf"
    commands = json.loads((build / "compile_commands.json").read_text())
    entries = [e for e in commands if e["file"].endswith("/src/core/dns.c")]
    if len(entries) != 1:
        raise SystemExit("A7B STOP: original SDK dns.c compilation identity missing")
    entry = entries[0]
    command = entry.get("arguments") or shlex.split(entry["command"])
    if any(x.startswith("-flto") for x in command):
        raise SystemExit("A7B STOP: LTO interception not admitted")
    object_path = Path(entry["directory"]) / command[command.index("-o") + 1]
    obj = run(["objdump", "-dr", "--disassemble=dns_send", str(object_path)])
    relocations = [line.strip() for line in obj.splitlines()
                   if re.search(r"\bR_\S+\s+udp_sendto(?:[-+]\S*)?$", line)]
    if len(relocations) != 1:
        raise SystemExit("A7B STOP: dns_send sole outbound relocation not proved")
    final = run(["objdump", "-d", "--disassemble=dns_send", str(elf)])
    calls = [line.strip() for line in final.splitlines()
             if re.search(r"\bcallq?\s", line)]
    redirected = [line for line in calls if "<__wrap_udp_sendto>" in line]
    if len(redirected) != 1 or any(re.search(r"<(?:__real_)?udp_sendto>", x) for x in calls):
        raise SystemExit("A7B STOP: final dns_send call redirection not proved")
    wrapper = run(["objdump", "-d", "--disassemble=__wrap_udp_sendto", str(elf)])
    allowed = {"xTaskGetCurrentTaskHandle", "pbuf_copy_partial", "memcpy", "__stack_chk_fail"}
    for line in wrapper.splitlines():
        if not re.search(r"\bcallq?\s", line):
            continue
        match = re.search(r"<([^>]+)>", line)
        if not match or match.group(1).split("@")[0] not in allowed:
            raise SystemExit("A7B STOP: wrapper contains unapproved call: " + line.strip())
    if "<__wrap_udp_sendto>:" not in wrapper:
        raise SystemExit("A7B STOP: wrapper body missing")
    print("A7B PROOF original-object: " + relocations[0], flush=True)
    print("A7B PROOF final-ELF: " + redirected[0], flush=True)
    print("A7B PROOF wrapper calls restricted to task identity / bounded memory copy", flush=True)

    # Read actual private dns.c macro definitions without changing SDK or objects.
    preprocessor = []
    skip = False
    for arg in command:
        if skip:
            skip = False
            continue
        if arg in ("-o", "-MF", "-MT", "-MQ"):
            skip = True
        elif arg not in ("-c", "-MD", "-MMD"):
            preprocessor.append(arg)
    macro_text = run(preprocessor + ["-dM", "-E"], cwd=entry["directory"])
    for hook in ("LWIP_HOOK_DNS_EXTERNAL_RESOLVE", "DNS_LOOKUP_LOCAL_EXTERN"):
        if re.search(r"^#define\s+" + hook + r"(?:\s|\()", macro_text, re.MULTILINE):
            raise SystemExit("A7B STOP: unproved alternate resolver hook " + hook)
    macros = dict(re.findall(r"^#define\s+(\w+)\s+(.*)$", macro_text, re.MULTILINE))
    keys = ("LWIP_DNS_SECURE", "DNS_TABLE_SIZE", "DNS_MAX_REQUESTS", "DNS_MAX_SOURCE_PORTS",
            "DNS_MAX_SERVERS", "DNS_MAX_RETRIES", "DNS_TMR_INTERVAL",
            "ESP_LWIP_DNS_TIMERS_ONDEMAND", "LWIP_TCPIP_CORE_LOCKING",
            "TCPIP_MBOX_SIZE", "LWIP_IPV4", "LWIP_IPV6", "LWIP_DNS")
    print("A7B_EFFECTIVE_DNS_MACROS " +
          json.dumps({key: macros.get(key, "ABSENT") for key in keys}, sort_keys=True), flush=True)
    print(run([command[0], "--version"]).splitlines()[0], flush=True)
    digest = hashlib.sha256(elf.read_bytes()).hexdigest()
    print("A7B_BINARY_SHA256 " + digest, flush=True)

    def invoke(mode, proven=False, expected=0):
        if hashlib.sha256(elf.read_bytes()).hexdigest() != digest:
            raise SystemExit("A7B STOP: tested binary changed after interception proof")
        env = dict(os.environ)
        env.pop("A7B_WRAP_PROVEN", None)
        env["A7B_MODE"] = mode
        if proven:
            env["A7B_WRAP_PROVEN"] = "verified-by-runner"
        result = subprocess.run([str(elf)], env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
        print(result.stdout, end="", flush=True)
        if result.returncode != expected:
            raise SystemExit(f"A7B STOP: {mode} exit={result.returncode}, expected={expected}")
        return result.stdout

    invoke("config")
    denial = invoke("success", expected=1)  # Must stop before TCPIP init / DNS registration.
    if "binary interception proof required; resolver NOT_RUN" not in denial:
        raise SystemExit("A7B STOP: unapproved invocation guard missing")
    invoke("smoke")  # No DNS registration in this mode.
    invoke("guard")  # Invalid capture rejected; still no DNS registration.
    print("A7B ZERO_EGRESS_PREEXECUTION_GATE PASS; resolver execution now admitted for this ELF only", flush=True)
    cases = ("immediate", "success", "error", "duplicate", "late", "fence-error", "exhaust")
    for case in cases:
        output = invoke(case, proven=True)
        if f"A7B SDK_DNS_HANDLER_RUNTIME {case} PASS" not in output:
            raise SystemExit("A7B STOP: missing case evidence " + case)
    print(f"A7B SDK_DNS_HANDLER_RUNTIME {len(cases)}/{len(cases)} cases PASS", flush=True)
    print("A7B NOT_RUN: native IP/UDP ingress, real DNS network, natural timer timeout, "
          "private found=NULL observation, permanent fence stall, token reuse, TLS/hardware", flush=True)

if __name__ == "__main__":
    main()
