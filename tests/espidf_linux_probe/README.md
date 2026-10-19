# Gate A6b / CLOSE-T1 — offline pinned SDK Linux runtime probe

Pins: ESP-IDF v6.0.2 `7101770dc6db2667b3c477cc31365dd1acd6db4e`,
esp-lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.

The existing ESP-IDF CI container builds the preview Linux target with
`CONFIG_LWIP_ENABLE=y`. It reports that target's generated sdkconfig independently
of C3/C6, including FreeRTOS mode, core-locking and mailbox settings, then runs the
linked ELF with a 20-second external watchdog. Each initialization/event/gate wait
has a 5-second test budget; the deliberately pending fence is checked with a
50-millisecond FreeRTOS wait. These are test watchdogs, not production deadlines.

Three in-memory cases execute real SDK `tcpip_init` and `tcpip_try_callback`:
nested callback/fence order on the same TCPIP task; a blocked callback and pending
fence across caller wait expiry with retained static storage; actual callback
admission/fence ERR_MEM under bounded queue saturation and subsequent drain.
ERR_MEM may come from message allocation or mailbox admission; the accepted count
and generated mailbox capacity are printed, without claiming allocator fault injection.
Core callbacks record events and signal SDK synchronization primitives; normal
traces print from the app task. These A6b cases make no netif, DNS lookup, socket
or TLS calls. The separately scoped T2A cases below add CLOSED BSD sockets only.

Owners remain static and are never recycled or freed. Process exit is not SDK
worker teardown. The exit marker is a test callback marker, not a DNS registration
clear event. The deferred fence is not proof of DNS callback quiescence.
DNS completion/cancellation, duplicate-host resolver behavior, never-arriving DNS,
safe token recycling, shutdown, connected close timing and hard caller-return bounds remain
NOT_RUN / UNPROVEN. Gate A feasibility stays PENDING, Gate B ON HOLD.

Initial feasibility evidence: candidate `09aee250e1b4f3c264539b4f0163aef9bd02fc25`,
Actions run `38590885407`, Linux job `115945125012`: build/link and one real
TCPIP callback smoke PASS. Linux generated configuration: lwIP enabled,
FreeRTOS SMP disabled, core locking disabled, thread-safety checking disabled,
mailbox 32, mDNS-query support enabled, DNS servers 3, SO_LINGER disabled.
This configuration evidence does not imply any DNS/mDNS query was executed.
The final three-case probe requires its own exact-candidate CI evidence.

## CLOSE-T1: actual CLOSED netconn lifecycle, not a whole-attempt deadline

The same ELF now retains all three A6b cases and adds two pinned-SDK cases:

1. Allocate `netconn_new(NETCONN_TCP)`, assert the actual PCB is `CLOSED`, and
   observe `netconn_delete()` return `ERR_OK`.
2. Allocate another CLOSED netconn before holding the real TCPIP task in a
   finite callback gate. A single static test worker calls `netconn_delete()`.
   Link-time observers delegate to **real** `sys_mbox_post` / `sys_arch_sem_wait`
   unchanged: successful mailbox admission and entry to the untimed semaphore
   wait are observed while the core remains held. No caller completion is
   allowed in the 50-ms observation window. Explicit release precedes actual
   delete completion; only `ERR_OK` permits clearing the owner handle.

Generated Linux configuration disables loopback; compile guards require
`LWIP_HAVE_LOOPIF=0`, `LWIP_NETIF_LOOPBACK=0`, and core locking disabled. Runtime
requires `netif_list == nullptr` before any netconn operation and after completion.
No bind/listen/connect/socket, DNS send, TLS, HTTP or SDK modification is used.
Run only with an isolated no-egress environment (`--network none`, source read-only).
SDKCONFIG stays in the build directory. The original CI 20-second ELF watchdog
and 5-second fixture waits remain; they are **test** budgets, not close deadlines.

Build with the pinned ESP-IDF Linux preview target, then run:

```sh
python -B tests/espidf_linux_probe/verify_close_t1.py /absolute/build-directory
```

The local runner first checks generated config and ELF routing of the unchanged
API-message path to observers and real SDK calls, then executes default success
and `WFRAC_T1_NEGATIVE=no-release` in separate bounded processes. The latter
must exit 2 with `TEST_TERMINATION`, retained owner and no T1 PASS 2. Its process
exit is **not** quiescence, cancellation or reclamation. Four additional synthetic
verifier negative tests reject incomplete/reordered traces, false termination
success and loopback config. Existing CI runs the default ELF (A6b 3 + T1 2);
it does **not** automatically execute this extra Python negative runner.

Owners and the one test worker/stack are static, finite, and retained through
true completion or process exit. There is no production worker or worker-reclaim
claim. Queue-saturated **delete** is NOT_RUN (the retained A6b callback saturation
is a different test). Connected FIN allocation/ERR_MEM, delayed FIN polling,
native socket close, TLS shutdown, HTTP cleanup, universal caller-return and
cleanup deadlines remain NOT_RUN / UNPROVEN. Gate A PENDING; Gate B HOLD.

## CLOSE-T2A: native never-connected BSD socket close / retained owner

The same pinned Linux ELF preserves A6b 3/3 and T1 2/2 and adds two cases:

1. A fresh `lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)` returns a valid
   descriptor; actual `lwip_close` returns 0 before owner invalidation.
2. Another fresh socket is retained in static storage while a finite test-only
   callback holds the TCPIP task. One static worker calls real `lwip_close`.
   The existing post/semaphore wrappers delegate unchanged to real SDK functions.
   During the 50-ms observation window the caller is blocked, has not returned,
   and retains the same descriptor. Explicit release precedes observed successful
   close return and only then owner invalidation. Post/wait counts are measured,
   not assumed to equal T1: socket disposal can require another completion.

Neither socket is bound, listening or connected. CLOSED is established by the
pinned fresh allocation path, not private PCB manipulation. After each close,
`lwip_fcntl(F_GETFL)` returns EBADF, independently showing descriptor-table
disposition. This is a separate invalid-descriptor API observation, **not** a
lower-layer close failure on a valid socket or timed proof of PCB reclamation.

Compile guards require IPv4/sockets, disabled loopback and no core locking.
Runtime requires `netif_list == nullptr` during allocation, held core, observed
post/wait and completion; no netif is ever created. Build/run locally in an
independently inspected Docker `--network none` container with read-only source
and SDK, dropped capabilities and no-new-privileges. That is local host-level
isolation evidence, not something inferred from `netif_list`. Existing GitHub
container routing does not itself establish host-level egress isolation; report
that remote limitation. No bind/connect/listen/accept/send/recv, DNS/TLS/HTTP or
hardware execution is performed. Source CMake does not change SDK configuration.

```sh
python -B tests/espidf_linux_probe/verify_close_t2a.py /absolute/build-directory
python -B tests/espidf_linux_probe/verify_close_t1.py /absolute/build-directory
```

T2A's local verifier proves generated Linux config, pinned actual socket source
build input, native ELF and close -> prepare-delete/free -> delete routing,
and real SDK post/wait observer delegation before executing the ELF. It runs
success and `WFRAC_T2A_NEGATIVE=no-release` separately under 20-second watchdogs.
The negative must exit 2 with retained owner and `TEST_TERMINATION`, with no close
return, owner invalidation or completion claim. No forced release/task deletion
is used. Twelve synthetic verifier tests (including ten missing-marker subcases)
reject incomplete/reordered/duplicate traces, premature owner release, missing
post/wait, changed descriptor, unsafe config, wrong call routing and termination
masquerading as success. Build post-link runs those verifier tests only; existing
callback CI runs the positive ELF, not the additional local negative runner.

Only the measured unconnected CLOSED socket normal-return and finite held/resumed
cases earn `SDK_RUNTIME_CONFIRMED`. Synthetic guards are `SYNTHETIC_PASS`.
Static workers/owners remain allocated until true completion or process exit;
process exit never proves teardown/reclamation. Connected FIN/TF_CLOSEPEND/ERR_MEM,
real close failure on a valid socket, TLS shutdown, DNS never-callback quiescence,
permanent-stall reclamation and aggregate hard caller-return/cleanup deadlines
remain NOT_RUN/UNPROVEN. Gate A PENDING/UNPROVEN; Gate B HOLD; Stage 4 Cold.

## CLOSE-F1: synthetic status / ownership counterexamples only

`tests/host_cpp/test_close_f1.cpp` is a standalone, allocation-free C++17 stand-in
of the pinned `esp_http_client` / transport close/destroy control-flow subset.
Its provenance comments refer to the same pinned source as
`evidence/inbox/stage3-gatea-cleanup-caller-return-source-20261017.md`.
It never links or executes actual SDK HTTP/TLS/socket code. A fictional handle
and injected release status demonstrate discarded failures; `unknown` means the
stand-in cannot establish resource disposition, **not** an observed real leak.

```sh
g++ -std=c++17 -Wall -Wextra -Werror tests/host_cpp/test_close_f1.cpp -o /absolute/output/close-f1
/absolute/output/close-f1
```

Eight named synthetic cases cover direct failure + owner loss, successful
response masking close failure, cleanup after lost handle, cleanup-only failure,
per-transport destroy status masking, preservation of a prior response error,
synthetic TLS-context owner loss, and positive release/keep-alive controls.
Results are `SYNTHETIC_PASS`, never native full-stack SDK or TLS evidence.
This standalone command is local validation, not registered in existing Host CI;
no workflow or Local CI manifest is changed. Actual close failure/ERR_MEM,
socket leak, worker safety and cleanup timing remain UNPROVEN / NOT_RUN.
