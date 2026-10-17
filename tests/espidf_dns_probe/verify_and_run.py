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

def verify_reuse_trace(output, case):
    """Check observed SDK trace order; synthetic self-tests are separate evidence."""
    def require(ok, label):
        if not ok:
            raise ValueError(label)

    owners = re.findall(r"^A7BR1_OWNER token=(\d+) slot=(\d+) generation=(\d+) "
                        r"address=(0x[0-9a-f]+) hostname=(\S+)$", output, re.M)
    rows = re.findall(r"^A7BR1_EVENT (\d+) (\w+) token=(\d+) owner=(\d+) "
                      r"generation=(\d+) hostname=(\S+)$", output, re.M)
    expected = [(0, 0, 1, "first.a7br1.test")]
    if case == "reuse":
        expected.append((1, 0, 2, "second.a7br1.test"))
    elif case in ("reuse-duplicate", "reuse-absent"):
        expected.append((1, 1, 1, "first.a7br1.test" if case == "reuse-duplicate"
                         else "absent.a7br1.test"))
    require(len(owners) == len(expected), "owner record coverage")
    require([(int(t), int(s), int(g), n) for t, s, g, _, n in owners] == expected,
            "exact token/slot/generation/hostname identity")
    require(len(rows) == len(re.findall(r"^A7BR1_EVENT ", output, re.M)) and rows,
            "trace record coverage")
    require([int(row[0]) for row in rows] == list(range(len(rows))), "trace sequence")
    for _, _, token, slot, generation, hostname in rows:
        require((int(token), int(slot), int(generation), hostname) in expected, "trace identity drift")

    def pos(token, event):
        hits = [int(row[0]) for row in rows if int(row[2]) == token and row[1] == event]
        require(len(hits) == 1, f"token {token} {event} missing/duplicate")
        return hits[0]

    for token, _, _, _ in expected:
        sequence = ["owner_acquired", "registration_INPROGRESS"]
        if case != "reuse-absent":
            sequence += ["original_dns_recv_enter", "DNS_found_enter",
                         "fence_rejected" if case == "reuse-fence-error" else "fence_admitted",
                         "DNS_found_exit_marker", "original_dns_recv_returned"]
            if case == "reuse-fence-error":
                sequence += ["queue_drained", "quarantined_reuse_rejected"]
            else:
                sequence += ["deferred_fence", "late_owner_quarantined" if case == "reuse-late"
                             else "owner_reusable"]
        indices = [pos(token, event) for event in sequence]
        require(indices == sorted(indices) and len(set(indices)) == len(indices), "SDK/barrier order")

    if case == "reuse":
        require(owners[0][3] == owners[1][3], "same owner storage not reused")
        require(pos(0, "owner_reusable") < pos(1, "owner_acquired"), "reuse before executed fence")
        pos(0, "stale_generation_guard_rejected")
        pos(1, "duplicate_callback_guard_rejected")
    elif case == "reuse-duplicate":
        require(owners[0][3] != owners[1][3], "duplicate owners alias")
        require(max(pos(t, "DNS_found_exit_marker") for t in (0, 1)) <
                min(pos(t, "original_dns_recv_returned") for t in (0, 1)) <
                min(pos(t, "deferred_fence") for t in (0, 1)), "coalesced callbacks/barrier order")
    elif case == "reuse-delayed":
        require(pos(0, "original_dns_recv_returned") < pos(0, "fence_held") <
                pos(0, "deferred_fence") and "A7BR1_HELD_FENCE_RETAINED PASS" in output,
                "held fence retained-state evidence")
    elif case == "reuse-late":
        require(pos(0, "caller_wait_expired") < pos(0, "DNS_found_enter") <
                pos(0, "late_result_discarded") < pos(0, "deferred_fence") <
                pos(0, "quarantined_reuse_rejected"), "late result quarantine")
    if case in ("reuse-absent", "reuse-fence-error", "reuse-late"):
        forbidden = {"owner_reusable"}
        if case == "reuse-absent":
            forbidden |= {"DNS_found_enter", "deferred_fence", "original_dns_recv_returned"}
            pos(0, "pool_exhausted_retained")
        if case == "reuse-fence-error":
            forbidden.add("deferred_fence")
        require(not any(row[1] in forbidden for row in rows), "unsafe release/callback evidence")

def trace_verifier_selftest():
    # Verifier-only fabricated records: never counted as SDK lifecycle execution.
    lines = []
    for token, generation, hostname in ((0, 1, "first.a7br1.test"), (1, 2, "second.a7br1.test")):
        lines.append(f"A7BR1_OWNER token={token} slot=0 generation={generation} address=0x100 hostname={hostname}")
    sequence = [(0, event) for event in ("owner_acquired", "registration_INPROGRESS",
                "original_dns_recv_enter", "DNS_found_enter", "fence_admitted", "DNS_found_exit_marker",
                "original_dns_recv_returned", "deferred_fence", "owner_reusable")]
    sequence += [(1, "owner_acquired"), (0, "stale_generation_guard_rejected")]
    sequence += [(1, event) for event in ("registration_INPROGRESS", "original_dns_recv_enter",
                 "DNS_found_enter", "fence_admitted", "DNS_found_exit_marker", "original_dns_recv_returned",
                 "deferred_fence", "owner_reusable", "duplicate_callback_guard_rejected")]
    for number, (token, event) in enumerate(sequence):
        hostname = "first.a7br1.test" if token == 0 else "second.a7br1.test"
        lines.append(f"A7BR1_EVENT {number} {event} token={token} owner=0 generation={token + 1} hostname={hostname}")
    valid = "\n".join(lines) + "\n"
    verify_reuse_trace(valid, "reuse")
    mutations = [valid.replace("deferred_fence", "missing_fence", 1),
                 valid.replace("original_dns_recv_returned", "deferred_fence", 1),
                 valid.replace("generation=2", "generation=1", 1),
                 valid.replace("address=0x100 hostname=second", "address=0x200 hostname=second"),
                 valid.replace("owner_reusable", "premature_reuse", 1),
                 valid.replace("DNS_found_exit_marker", "DNS_found_enter", 1)]
    for invalid in mutations:
        try:
            verify_reuse_trace(invalid, "reuse")
        except ValueError:
            continue
        raise SystemExit("A7BR1 STOP: trace verifier accepted malformed synthetic evidence")
    print("A7BR1 VERIFIER_SELFTEST 7/7 PASS; synthetic verifier checks only, SDK NOT_RUN", flush=True)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("build", type=Path, nargs="?")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    trace_verifier_selftest()
    if args.self_test:
        return
    if args.build is None:
        parser.error("build is required for SDK verification")
    build = args.build.resolve()
    config = (build / "sdkconfig").read_text().splitlines()
    if ("# CONFIG_LWIP_NETIF_LOOPBACK is not set" not in config or
            "CONFIG_LWIP_NETIF_LOOPBACK=y" in config):
        raise SystemExit("A7B STOP: generated SDK loopback not disabled; resolver NOT_RUN")
    print("A7B_CONFIG generated CONFIG_LWIP_NETIF_LOOPBACK=disabled", flush=True)
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

    binary_config = invoke("config")
    if not re.search(r"\bLWIP_HAVE_LOOPIF=0\s+LWIP_NETIF_LOOPBACK=0\b", binary_config):
        raise SystemExit("A7B STOP: binary loopback macros not zero; resolver NOT_RUN")
    for mode in ("success", "reuse"):  # Stop before TCPIP init / DNS registration.
        denial = invoke(mode, expected=1)
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
    reuse_cases = ("reuse", "reuse-duplicate", "reuse-late", "reuse-fence-error", "reuse-delayed", "reuse-absent")
    for case in reuse_cases:
        output = invoke(case, proven=True)
        if f"A7BR1_COMPLETED_DNS_OWNER_REUSE {case} PASS" not in output:
            raise SystemExit("A7BR1 STOP: missing case evidence " + case)
        try:
            verify_reuse_trace(output, case)
        except ValueError as error:
            raise SystemExit(f"A7BR1 STOP: {case} trace: {error}") from None
    print(f"A7BR1 COMPLETED_DNS_OWNER_REUSE {len(reuse_cases)}/{len(reuse_cases)} cases PASS; "
          "static owner-slot reuse only; generation callback records retained", flush=True)
    print("A7B NOT_RUN: native IP/UDP ingress, real DNS network, natural timer timeout, "
          "private found=NULL observation, permanent fence stall, arbitrary token free/recycling, "
          "caller-return/cleanup deadlines, TLS/hardware", flush=True)

if __name__ == "__main__":
    main()
