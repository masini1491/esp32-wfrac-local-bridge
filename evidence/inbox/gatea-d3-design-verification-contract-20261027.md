# Gate A / D3 — Offline fake-transport design-verification contract

Date: 2026-10-27 (Asia/Taipei)
Stage: `GATEA-D3_DESIGN_VERIFICATION_OFF_REPO_FAKE_TRANSPORT_USER_AUTHORIZED_20261027` under sole parent Hot `GATEA-D2`.
Authority: user's 2026-10-27 instruction to **continue** after the specifically proposed "GATEA-D3 Design Verification Only" bounded tranche. This authorizes one **offline software-only evidence/verification tranche**, not production implementation, SDK build, CI, network, real hardware, source publishing or Gate A promotion.
Owner split: ChatGPT has completed scoped vendor/source research and canonical contract admission. Codex owns any residual reproducible **off-repository fake-seam model/harness code and host-only test execution**, returning immutable evidence to ChatGPT for independent acceptance.
Pinned research: [initial SDK assessment](gatea-d3-pinned-sdk-total-deadline-feasibility-20261027.md), [custom-transport architectural decision](gatea-d3-custom-transport-architecture-decision-20261027.md).
Frozen inputs: `masini1491/esp32-wfrac-local-bridge@c7210cea467d391cf008322ee593c1b5db1f3367`, `masini1491/ai-development-playbook@29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70d`, vendor `espressif/esp-idf@7101770dc6db2667b3c477cc31365dd1acd6db4e`. New TASKS/evidence-only commits are **not retests of source**. Reconcile newer canonical revisions without silently substituting any SDK or project source.

## Goal and falsifiable questions

Determine what a common single-owner monotonic-deadline-aware custom `esp_transport` could establish under *synthetic* HTTP and HTTPS fake-seam tests, and which claims are **not established** by the actual pinned vendor API. Need a clear **GO-to-next-native-probe**, **NO-GO**, or **INCONCLUSIVE** research decision; a fake-only PASS alone must never justify implementation of a physical deadline.

Evidence must address: (1) the genuine callback seam coverage in the exact pinned ESP-IDF `esp_http_client.c`, including internal bypasses, synchronous callback behavior and event posting; (2) Kconfig gate `CONFIG_ESP_HTTP_CLIENT_ENABLE_CUSTOM_TRANSPORT` (default n) and `CONFIG_ESP_HTTP_CLIENT_EVENT_POST_TIMEOUT` (default 2000ms); (3) per-request and transport ownership/destroy ordering (custom transport is not automatically proof of transfer or teardown); (4) secure TLS PEM/CA hostname verification & no downgrade; (5) numerically addressed vs DNS endpoint scope; (6) low-level plain-TCP connect readiness `EINPROGRESS`/SO_ERROR semantics, and `is_async` documented HTTPS-only.

### Executable *model-only* test matrix

Do **not** simulate a different API and label it actual vendor integration. Harness should explicitly identify `FAKE_TRANSPORT_MODEL_ONLY` for every executable case. Each **applicable** case must assert the expected **named invariant/guard and final ownership state**, not merely nonzero exit or arbitrary exception.

For **both HTTP and HTTPS models**, cover at minimum:
- Baseline complete response accepted within budget, monotonic `T0` + single absolute attempt deadline.
- Header trickle; Content-Length body trickle; chunked body trickle. Each successive positive read must consume from the **same** budget; expired budget stops future I/O.
- Connect delayed beyond remaining budget; stalled read; stalled write; partial write then timeout with delivery `UNKNOWN` (no auto-retry, even if caller is uncertain).
- Buffer overflow with bounded failure classification; no subsequent accepted success or unbounded I/O.
- Early EOF/incomplete/malformed response failure.
- Sticky expiry, zero/negative remaining budget, backward-moving injected clock fail-closed.
- STOP followed by attempted read/write/poll/connect: **no I/O after STOP**.
- Late/duplicate callback, terminal decision followed by queued event, and callback-after-release attempt: detect invalid ordering, refuse resource release until quiescence is justified.
- Double release, cleanup failure or aliased resource ownership: fail closed rather than falsely claim teardown `PASS`.

For **HTTPS only**, add fake handshake latency exceeding budget, TLS certificate/host mismatch (no trust bypass, HTTP fallback or optimistic success), and missing/invalid PEM guard. For **HTTP plain TCP**, add `connect()→EINPROGRESS` followed by failed/pending `SO_ERROR` readiness: do not classify connected prematurely.

Required **negative meta-controls**: invalid baseline must never count as proof-negative PASS; mutation that is silently accepted must fail the harness; a wrong named guard cannot count as rejection success; skipped/absent actual test backend or accidental stub-only success cannot count as SDK behavioral proof.

## Architectural guardrail checklist, separate from model tests

Mark each as `SUPPORTED_BY_PINNED_SOURCE` / `UNPROVEN` / `CONTRADICTED`, with file/line and source identity:
1. All `esp_http_client` built-in header/body connect/read/write paths traverse injected transport.
2. Nontransport `esp_event_post` waits bounded within total budget, including repeated event posts (default wait may exceed budget).
3. All TLS handshake and host resolution steps honor the remaining deadline; runtime and thread quiescence proven across abort.
4. Plain-HTTP async readiness truly checks completion; no premature connected state.
5. HTTPS custom transport receives explicit authenticating trust/CN configuration (do not infer automatic inheritance from internal SSL transport).
6. Custom transport and client state are freed once, after terminal callback quiescence, with no event-loop use-after-free and no unbounded retained worker/state.
7. `esp_http_client_cancel_request` cannot be assumed a safe cross-thread cancel: pinned implementation closes **then reconnects**.
8. `ResponseCollector` callback returning error is **not** guaranteed to abort SDK parser: `http_on_body` does not propagate the handler's return.

Any unproven guard here must remain **UNPROVEN** even when the fake model says PASS. No unsupported claim of hard total physical deadline, `D_physical`, `D_cleanup`, or hardware/CI/SDK compile evidence.

## Execution boundaries and artifacts

- Before local execution, follow `REPOSITORY_EXECUTION.md` identity/permission/safe-sync bootstrap; Git ownership error / unchanged `CreateProcess os error 5` is a **STOP**, not permission to bypass guards or try another privileged context. If verified exact source materialization already exists under an authorized path, prefer it over needlessly resyncing. No changes to repository files by Codex.
- One new uniquely identified root under a checked **off-repository** temp location, also outside Stage 1/2/2R preserved evidence and all linked worktrees; no overwrite, deletion, re-extraction into existing preserved root, source checkout modifications, Git commit/push/PR/Actions.
- Only sterile fixture source, host-only script/harness and generated evidence/patches under that new root. Offline; **zero child processes except the explicitly authorized host-only harness executor/toolchain**. No SDK native build/link, C6/C3 compile, external HTTP/TLS/socket/DNS, network services, production or physical hardware.
- One initial aggregate fake-model verification run; permit **at most 3 total** uniquely labelled attempts if a true test-harness defect is corrected, **up to 120 seconds each**, archive every attempt's stdout/stderr/exit/elapsed, per-case named outcomes, source/harness SHA-256, process-attempt count, and before/after preservation hashes. No unlimited retry, no silent re-run, no unbounded worker. Source identity, permission, unknown root cause, test-scope integrity or unexpected artifact mutation failure → immediate STOP.
- Handoff: exact fake-model source/test script, a manifest of every file and SHA-256, exact source pins, 2-scheme case-level JSON, negative meta-controls, static vendor line-scoped guard table, recorded command/executor/CWD/exit/stdout/stderr, all FAIL and PASS runs, explicit `GO_TO_NEXT_NATIVE_PROBE` / `NO_GO` / `INCONCLUSIVE` with rationale, and all still-`UNPROVEN` dimensions. Source/readback must be independently reproducible or report `INCONCLUSIVE`.
- Codex **STOP** after returning reviewable artifacts; ChatGPT independently audits and decides whether *this design-verification stage* has closure. No automatic admission to source implementation or SDK experiment.


## Root-recovery corrective amendment — 2026-10-27 (user-authorized, active limited override)

**Current execution authority is the sole \`GATEA-D2\` Hot in \`TASKS.md\`; this is an annex to the same \`GATEA-D3_DESIGN_VERIFICATION_OFF_REPO_FAKE_TRANSPORT_USER_AUTHORIZED_20261027\` child, not a fresh Stage.** User explicitly authorized "執行修復" after Codex returned \`ROOT_MISSING\` on an invocation-specific sandbox TEMP directory. Original path's inaccessibility in tested contexts is recorded, **not** claimed as global deletion. No harness attempt was made, so original aggregate harness capacity remains **0/3 consumed; three total runs maximum, each <=120s**. This amendment supersedes **only** the earlier requirement that the first (now inaccessible) transient root remain the sole usable root. The failed original root is historical and must not be overwritten, recreated, moved, modified or cleaned up.

**One-and-only-one stable replacement root is now permitted**, conditional on trusted preflight:
- Candidate literal: \`%LOCALAPPDATA%\wfrac-gatea-d3-evidence-20261027-recovery-01\` (expand in the authorized Windows user context and pin the resulting **absolute path**). A unique name may be selected exactly once **before creation** if the literal is demonstrably unsuitable; do not try multiple names or roots. \`%TEMP%\`, \`%TMP%\`, per-invocation sandbox GUIDs, repository roots, linked worktrees, all preserved SDK/Stage 1/2/2R roots, and source checkout subtrees are prohibited. Never infer the same storage because strings look similar.
- Before creation, independently verify the selected parent and candidate canonical path, that it is not under any protected tree, candidate is absent, user principal owns/has appropriate unprivileged read/write access, and no reparse-point/symlink/junction alias can escape the intended tree. If any identity, ACL, path existence, or disjointness fact cannot be established, **STOP without creation**. Do not change Git safe.directory, ACL, sudoers, privileges, ownership or execution identity to make it pass.
- Command-context A: create exactly the one selected root **once**, write a minimal fixed-path \`root-probe.json\` containing random nonce, expected stage identity, root's absolute canonical path, executing user SID/principal, creation context, and file-content hash (self-hash circularity excluded). Independently record file SHA-256, length, and root file-system identity/attributes where possible. This is **environment preflight**, not a fake-model aggregate run.
- Command-context B: in a **separate command execution**, using the **exact canonical absolute path persisted by A** (not a new \`$env:TEMP\` or re-expanded dynamic sandbox path), read the existing marker without recreating any path and verify nonce, bytes, SHA-256, identity, physical path, access and non-reparse boundaries. Check Windows principal and execution context equivalence and preserve a concise \`root-continuity-receipt.json\` only after both observations agree. The marker's existence in A alone cannot count as persistent storage proof. If B returns \`ROOT_MISSING\`, different bytes, different principal/namespace, access denied, or unknown result: **STOP**, preserve observations, **do not** create root number three or consume any harness attempt.
- Once A+B pass, the accepted replacement becomes the **sole active evidence root** for the already-authorized D3 fake-only model. All subsequent commands must use this literal stable absolute path, recheck existence/marker before each authorized harness attempt, and keep evidence, source, run logs, manifest, ZIP and all FAIL history inside it. Do not write source or launch Python until A+B pass.
- After successful preflight, original D3 fake-model test contract (two scheme profiles, named guards, offline no-network/no-SDK/no-hardware, one initial aggregate + at most two further source-attributed bounded corrective attempts, preserved separate stdout/stderr/exit) resumes **without a new campaign**, and Codex returns all original evidence for ChatGPT independent review. This permission does **not** authorize source/tracked-file edits, SDK native campaign, Git push/PR/Actions, GitHub mutation, external requests, child process experimentation or Stage promotion.
- Report **environment recovery PASS**, **environment recovery STOP**, and **test PASS** separately. The recovery marker/receipt cannot be counted as a fake-model case or be used to infer runtime physical deadline or TLS safety.

If the old transient root later becomes accessible, leave it untouched and report it as historical; do not merge or replace active root on the fly. All old preservation and Gate A / Gate B HOLD constraints remain fully in force.

## Preservation

Stage 2R: `STAGE2R_TEST_ONLY_CORRECTIVE_ACCEPTED__TRACE_LIMITED` CLOSED. Historical Stage 2 HTTPS positive-map mismatch `UNRESOLVED`; original 29 staged candidates and all preserved SDK artifacts untouched. Gate A `PENDING/UNPROVEN`, Gate B `HOLD`, Stage 4 `COLD`, library readiness `PRESERVED`. 
