# Gate A3 — disposable offline model preflight (2026-10-14)

**Classification:** preliminary `OFFLINE_SIMULATED` / research-only; NOT the admitted Codex test-candidate validation, not a real pinned SDK/lwIP runtime result, not Gate A/B PASS. Current Hot coordination owner: `TASKS.md → Hot — Health audit Stage 3 → Gate A3`.

**User authorization:** `執行` following proposed Gate A3 separate offline validation. Scope is synthetic model and authorized test-only handoff; no production, DNS network operations, TLS handshake, flash or hardware. `ChatGPT+Codex`: ChatGPT conducts independent research/coordination, Codex makes tracked test/CI mutations.

**Provenance / boundaries:** target before admission `239fb417a58c0c9f66cdaf9657539b8ec8efd051`; shared Playbook `bc43725aeda50c3540ab7ef320e31076843b9c1e`; pinned SDK `espressif/esp-idf@7101770dc6db2667b3c477cc31365dd1acd6db4e` and lwIP `espressif/esp-lwip@fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e` are source-reviewed **but were not executed** in this preflight. Source/known unknowns: [Gate A research](stage3-gatea-hostname-tls-cleanup-20261013.md).

**Disposable execution:** In a temporary path outside the tracked repository, executed `python -B /tmp/wfrac_a3_offline_model.py` (Python `unittest`, built-in only) on a toy fixed-slot resolver lifecycle model. **16 cases, 16 PASS, zero failures, zero skips**. The temporary model file is **not committed or reproducibly archived** in the repo; do not promote this result to CI, tested implementation, independent source-backed concurrency or a reusable regression baseline.

Cases actually modeled:
1. immediate OK
2. immediate ERR_MEM
3. immediate ERR_ARG
4. timeout retains outstanding slot
5. late success ignored
6. late failure ignored
7. callback success before owner finish
8. callback active during timeout blocks slot reuse
9. duplicate callback entrance guarded
10. generation token detects stale callback after reuse
11. full capacity exhaustion
12. distinct slots for simultaneous hostname queries
13. simulated cache-clear callback failure after timeout
14. scheduler stall / failover never callback → slot retained
15. dispatch failure before registration releases slot
16. cleanup overrun leaves pending slot blocked

**What passed:** The *toy implementation's* manually programmed ownership transitions and rejection checks matched these assertions, demonstrating the intended test oracle is expressible and revealing that fixed allocation bounds memory while allowing persistent slot exhaustion. It did **not** test a real DNS client, true threads/interleavings, system memory lifetime, safety of reusing a raw `void*` after a callback, lwIP TCPIP-core lock behavior, real `tcpip_callback` scheduling, actual `close()` return latency, cert/SNI binding or bounded caller return. A token/generation check does not make a freed address safe to dereference.

**Next admitted proof:** Codex creates a minimal tracked synthetic host test candidate with deterministic lifecycle state/event schedule, actual failing negative controls and explicit resource safety / unresolved-liveness claims; exact candidate Host CI, review and read-back remain required before A3 acceptance. Fail-closed classifications when callback never arrives or close has no bound. Do not conflate 16 disposable tests with accepted D1 36/36 existing parser/fake tests. Gate A implementation feasibility PENDING; Gate B ON HOLD; Stage 4 Cold. Library readiness PRESERVED.
