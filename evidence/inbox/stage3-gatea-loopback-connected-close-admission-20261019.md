# Stage 3 Gate A — connected-close admission after CLOSE-T2A

Date: 2026-10-19 (Asia/Taipei). Baseline: `main@0380513dfe49625a318dde30cb9a041eaaf83adb`.
Pinned: ESP-IDF `7101770dc6db2667b3c477cc31365dd1acd6db4e`; esp-lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.
Evidence: pinned SOURCE_CONFIRMED, prospective test feasibility INFERRED. No new compile/runtime/network/Codex validation is claimed.

## Established facts and what they cannot prove

PR #19 is MERGED. Main Host CI [39192571664](https://github.com/masini1491/esp32-wfrac-local-bridge/actions/runs/39192571664) succeeded. A6b (3/3), T1 (2/2) and T2A (2/2) proved only their respective pinned Linux paths; T2A used never-connected CLOSED sockets and proved caller-return/owner order in finite cases. D1 36, D2-P0 26, D2-P1 14 include fake/synthetic tests; C3/C6 six-cell mbedTLS tests are SDK_COMPILE_ONLY. None proves real connected FIN/ERR_MEM, native TLS teardown, valid-socket close failure, pending DNS callback reclamation or full L1/L2/L3.

The current [adapter](../../components/wfrac_transport_espidf/src/esp_http_transport.cpp) still calls blocking `esp_http_client_perform()` and `esp_http_client_cleanup()`, without real aggregate controllable-I/O checkpoint integration or independently observed failed close. Merely repeating disconnected BSD socket tests would not substantively reduce this gap. Accepted D0 does *not* promise an unconditionally hard synchronous caller-return or literal zero residual TCP PCB.

## Bounded source feasibility for genuine connected socket test

- Pinned [ESP-IDF Kconfig L569–582](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/lwip/Kconfig#L569-L582) declares `CONFIG_LWIP_NETIF_LOOPBACK`.
- Pinned [lwipopts.h L800–820](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/lwip/port/include/lwipopts.h#L800-L820) maps it into `LWIP_HAVE_LOOPIF` and `LWIP_NETIF_LOOPBACK`. Pinned [esp-lwIP netif.c L190–215](https://github.com/espressif/esp-lwip/blob/fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e/src/core/netif.c#L190-L215) configures a local loopback netif; the same file [L1120–1250](https://github.com/espressif/esp-lwip/blob/fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e/src/core/netif.c#L1120-L1250) provides in-stack packet-loopback handling.
- This is **not** proof that a reproducible pinned ESP-IDF Linux build with internal TCP peer works, nor that connected FIN or ERR_MEM can be triggered deterministically. No new runtime was performed.
- Existing [A6b/T1/T2A Linux fixture](../../tests/espidf_linux_probe/README.md) explicitly disables loopback, compile-rejects `LWIP_HAVE_LOOPIF`, and requires `netif_list == nullptr`. It must not be silently reconfigured: doing so breaks the assumptions for the accepted tests.
- Project [AGENTS.md](../../AGENTS.md) prohibits actual network runtime without independent explicit permission. Even a Docker `--network none` loopback test is TCP network runtime. Docker isolation evidence must be established independently before claiming no external egress.

## Admission decision, not new execution authority

**Recommended conditional NEXT:** separately approve `CLOSE-T3-FEAS`, an isolated Linux test-only *separate* loopback build/fixture and no-egress safety preflight, without changing normal A6b/T1/T2A. First prove pinned CMake/Kconfig/link and host isolation, then stop at the new explicit runtime gate. If granted, only then propose a narrower connected loopback `CLOSE-T3` native positive test and independently verified callback/socket/FIN observations. A deterministic `ERR_MEM` fault must NOT be assumed or forged by private PCB/SDK patching.

Alternatively, separately approve a private D2 production adapter candidate while retaining Gate A as UNPROVEN; offline Host/C3/C6 green alone cannot resolve connected L3 close. Neither route presently admitted by the user's broad continue instruction.

Do not launch new Codex test execution, modify production/SDK/workflows, connect to LAN/Internet/WF-RAC, use secrets/hardware, remove TLS authentication or infer timer-based PCB reclamation from `close()==0`. Do not declare Gate A PASS. Stage 3 sole Hot, Gate A PENDING/UNPROVEN, Gate B HOLD, Stage 4 Cold, Library readiness PRESERVED.
