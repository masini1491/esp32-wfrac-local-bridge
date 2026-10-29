# Unfinished work queue

> **Current Hot coordination only.** This file is not an execution grant, test log, or replacement for canonical technical/evidence owners. The exact unabridged pre-cleanup queue is immutable at [TASKS.md@6b66a469](https://github.com/masini1491/esp32-wfrac-local-bridge/blob/6b66a469f2683800939e8ab98e460dbdf9cac63a/TASKS.md); [earlier 2026-10-22 compaction](https://github.com/masini1491/esp32-wfrac-local-bridge/blob/bbc978514db555cdc6707e464930fd2f9b3af80b/TASKS.md) is also retained. Historical approvals, STOPs and commands are **not current authorizations**.

## Sole Hot — Stage 3 / `GATEA-D2` (P2 aggregate HTTP attempt deadline)

**Current:** `GATEA-D2` remains the only Hot. Latest child `GATEA-D4_PINNED_SDK_CUSTOM_TRANSPORT_COMPILE_LINK_ONLY_USER_AUTHORIZED_20261027` is **CLOSED** at `D4_NATIVE_C6_CUSTOM_TRANSPORT_COMPILE_LINK_SCOPED_ACCEPTED__CHILD_CLOSED`. **Current newly authorized bounded test-only child:** `D2_SDK_TRANSPORT_LIFECYCLE_V2_CONTAINER_INSPECT_ACCEPTED__START_NOT_RUN` (one pinned SDK C transport dispatcher host test; no real network, HTTP/TLS, production implementation, or protected-volume mounts).

**Project-wide boundary:** Gate A **`PENDING/UNPROVEN`**; Gate B **`HOLD`**; Stage 4 **`COLD`**; library readiness **`PRESERVED`**. No network, HTTP/HTTPS/TLS execution, hardware, ELF execution, source/SDK mutation, CI or Docker action is authorized by this queue cleanup.

### Unresolved Gate A critical path

| Gate | Present evidence | Missing proof / stop condition |
| --- | --- | --- |
| **L1** — one shared monotonic budget across *controllable* I/O | D0/D1 offline contract; D3 fake-only model; D4 inert native compile/link | Real HTTP/HTTPS controllable connect/read/write/poll and trickle paths must enforce one remaining budget. Current `esp_http_client_perform()` uses per-operation `timeout_ms`; no SDK-runtime aggregate proof. |
| **L2** — fail-closed terminal result and one physical attempt | Host/fake atomicity and negative-control evidence | Real SDK failure, partial body, overflow and possibly-delivered write must never publish premature success, blindly retry or downgrade TLS. |
| **L3** — safe single owner, observed quiescence and release | Partial SDK close/DNS research and D3 fake ownership cases | `D_result`, `D_physical`, `D_cleanup`, callback fence, TLS/socket close and unknown-cleanup quarantine need credible native behavior evidence. Do not infer physical reclaim from `esp_http_client_cleanup()==ESP_OK`. |

**D0 acceptance nuance:** The accepted contract does **not** demand universal hard synchronous caller-return or literal zero residual TCP PCBs. It permits disclosed unpreemptible SDK/cleanup overrun, but still requires L1 checkpoints for *controllable* I/O, L2 atomic fail-close and L3 safe ownership/quarantine. [D0 scope reconciliation](evidence/inbox/stage3-gatea-d0-scope-reconciliation-20261018.md) and [HTTP subcall/cleanup analysis](evidence/inbox/stage3-gatea-d0-http-subcall-and-atomic-cleanup-20261018.md) govern this interpretation.

### Completed / bounded historical children — DO NOT RE-RUN

| Item | Final scoped disposition / remaining limitation | Evidence |
| --- | --- | --- |
| Original D2 Stage 1 archive | Integrity accepted at archived-evidence tier; not SDK runtime acceptance. | [Stage 1 independent review](evidence/inbox/stage3-gatea-d2-stage1-integrity-independent-review-20261022.md) |
| Original D2 single SDK campaign | **CONSUMED**; build exit 0, verifier exit 1, original HTTPS positive map contaminated by negative link; **not** full campaign PASS. Historical outputs/D2 volume remain preserved. | [Map integrity blocker](evidence/inbox/stage3-gatea-d2-single-campaign-map-integrity-blocker-20261021.md) |
| Stage 2R parser corrective | `STAGE2R_TEST_ONLY_CORRECTIVE_ACCEPTED__TRACE_LIMITED`; archived run04 reports 96/96 synthetic/focused PASS with independent archive verification; distinct stdout/stderr unavailable, historical truncated Playbook SHA retained. All authorized offline harness attempts exhausted; no rerun. | [Exact scoped decision in immutable TASKS](https://github.com/masini1491/esp32-wfrac-local-bridge/blob/6b66a469f2683800939e8ab98e460dbdf9cac63a/TASKS.md) |
| D3 Fake Transport model | `D3_FAKE_MODEL_ONLY_VERIFICATION_ACCEPTED__SDK_FEASIBILITY_INCONCLUSIVE`; recorded 53/53 fake cases and 8/8 meta-controls accepted at model/archive tier. **3/3 D3 attempts consumed**. | [D3 archive acceptance](evidence/inbox/gatea-d3-attempt03-independent-acceptance-20261027.md) |
| D4 pinned native C6 Custom Transport | `D4_NATIVE_C6_CUSTOM_TRANSPORT_COMPILE_LINK_SCOPED_ACCEPTED__CHILD_CLOSED`; attempt01 guest/container exit 0, ELF32 RISC-V, final ELF 5/5 symbols defined, three consumer relocations, map/linker provenance and pinned ELF/map hashes. **1/2 D4 native attempts used**; attempt02 not approved or needed. *Inert fixture only; no real SDK HTTP/TLS/runtime/cleanup result.* | [D4 final scoped acceptance](evidence/inbox/gatea-d4-attempt01-exact-execution-admission-20261029.md) |

### Next decision / preserved boundaries

**Current next test-only tranche (USER AUTHORIZED; HOST NOT_RUN):** use the [single pinned SDK transport lifecycle test-only admission](evidence/inbox/gatea-d2-sdk-transport-lifecycle-testonly-admission-20261029.md) to inspect real `transport.c` callback dispatch, close-error return, destroy order, and custom handle versus list ownership. Off-repo kit SHA256 `a0289998412a56a6913a02b4e501f01a6abc062f1580d84f47d5850655ec3b6e (v2; supersedes unrun v1)`; static/synthetic test-only checks PASS but actual user WSL/Docker run NOT_RUN. Then reconsider the broader SDK-behavior and ownership-observability proof for the existing D0-compatible Custom Transport direction. Decide exact API/HTTP+HTTPS paths, callback/event-post/hostname/TLS identity and close/quiescence witnesses, with fail-closed STOP criteria before any new test/implementation admission. [D2 ownership design](evidence/inbox/stage3-gatea-d2-p0-controllable-io-owner-design-20261018.md) · [D3 custom transport decision](evidence/inbox/gatea-d3-custom-transport-architecture-decision-20261027.md).

**Not authorized:** Any new SDK/native/Docker campaign outside this exact user-authorized one-shot D2 test-only container with no network or volume mounts; D4 attempt02, D3 attempt04, Stage 2R harness rerun, new SDK/native/Docker campaign, production source change, real HTTP/TLS/network/hardware, CI, Gate B or Stage 4 promotion. Do not mount, delete, rewrite or reset protected D2 volume, D4 volume/container, preserved LocalCache fixture, ELF/map or historical receipts; no ACL/ownership/Git trust bypass. Any new execution needs a separate explicit user authorization and fresh repository/worktree/permission preflight.

**Cold registry:** [BACKLOG.md](BACKLOG.md) retains Stage 4 and other dormant candidates; their presence is not launch authority.

## Historical traceability

This is a **coordination-only compaction**, not validation, source modification, fresh local worktree check or state promotion. Full Stage 2R/D3/D4 admission steps, immutable historic statuses, exact SHA pins, evidence and STOPs remain in [the immediate pre-cleanup `TASKS.md` at `6b66a469`](https://github.com/masini1491/esp32-wfrac-local-bridge/blob/6b66a469f2683800939e8ab98e460dbdf9cac63a/TASKS.md) and their linked `evidence/inbox/` files. New readers must use the current Hot summary above rather than treating old commands as active.
