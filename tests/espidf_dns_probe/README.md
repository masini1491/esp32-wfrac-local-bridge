# Gate A7b — intercepted Linux SDK DNS handler tests

Pinned ESP-IDF v6.0.2 `7101770dc6db2667b3c477cc31365dd1acd6db4e`;
esp-lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.
This isolated fixture does not change A6b, SDK, production or firmware sources.

Before DNS registration, `verify_and_run.py` fails closed unless the original
SDK dns.c object has the expected udp_sendto relocation, the final ELF dns_send
calls __wrap_udp_sendto, and the wrapper has only allowlisted task/memory calls.
LTO is rejected. It reports actual dns.c preprocessor definitions and compiled
configuration, hashes the binary, checks a missing-proof rejection, then runs
wrapper-only capture/invalid-argument smokes. All resolver modes require this gate.
The environment marker prevents accidental invocation; it is not a security
sandbox or independent cryptographic attestation. Binary identity is rechecked
by the runner before every process invocation.

The wrapper captures bounded query bytes/PCB metadata and returns without
forwarding. No interface is attached and netif_list must remain null. There are
no native socket calls, real packet output, TLS, QEMU or hardware operations.
A query is formed internally by real SDK dns_enqueue/dns_send; zero egress is
asserted at this fixture's lwIP UDP boundary, not as an OS-wide network guarantee.

Seven separate-process cases cover immediate numeric/cached ERR_OK, ERR_ARG and
absent-server ERR_VAL; injected success and error; coalesced duplicate hostname
with distinct static arguments (requires observed secure configuration); late
completion after a 50-ms caller wait expiry with a delayed fence; actual fence
ERR_MEM under bounded queue saturation; no injected callback, retained owners
and registration exhaustion. SDK waits use 5-second test budgets, pending checks
50 ms and each host executable a 20-second watchdog. These are not production
deadlines. ERR_MEM does not distinguish allocator failure from mailbox admission.

Replies exist only in RAM and are passed to the genuine udp_pcb recv callback,
bypassing native IP/UDP ingress. SDK owns the response pbuf. Captured PCB/recv
references are retired before calling recv and never dereferenced afterward.
Observe SDK_DNS_HANDLER_RUNTIME only: callback entry/exit marker, original
dns_recv return and subsequent real TCPIP fence. Private dns_requests[].found
is not read; clear-after-callback remains a pinned-source fact. Cached lookup is
only an external cross-check. No token is freed/reused and every owner remains
static until process exit; process exit is not SDK teardown.

NOT_RUN / UNPROVEN: real network DNS, IPv6 responses, natural timer timeout,
permanent never-running fence/callback, scoped cancellation, private registration
clear observation, token reclaim/reuse, close timing, caller-return bound and
device parity. No manual dns_tmr or dns_clear_cache is used. Gate A feasibility
remains PENDING, Gate B ON HOLD; independent acceptance is required before merge.
