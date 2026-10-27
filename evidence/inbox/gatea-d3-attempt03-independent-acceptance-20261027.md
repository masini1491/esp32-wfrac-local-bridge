# GATEA-D3 — final attempt-03 independent source/archive acceptance

Date: 2026-10-27 (Asia/Taipei)  
Repository: `masini1491/esp32-wfrac-local-bridge`  
Sole parent Hot: `GATEA-D2`; child: `GATEA-D3_DESIGN_VERIFICATION_OFF_REPO_FAKE_TRANSPORT_USER_AUTHORIZED_20261027`  
**Decision:** `D3_FAKE_MODEL_ONLY_VERIFICATION_ACCEPTED__SDK_FEASIBILITY_INCONCLUSIVE` — **child CLOSED at FAKE_TRANSPORT_MODEL_ONLY evidence tier**, **not** Gate A PASS, actual deadline proof, native SDK integration, or production readiness. This is a scoped archival and negative-oracle acceptance, not an approval to execute additional tests.

## Input and independent artifact integrity

User-uploaded immutable final file: `handoff-corrective-03-final.zip`. Its bytes were directly inspected in ChatGPT's container, **without running any bundled script**.

- ZIP: **150,973 bytes**, SHA-256 `e003298f94829fe618ae6e76fae84c7bffc86647d9f213cfa01f09a944e1f382`, **50 unique regular members** (no duplicate paths), ZIP CRC PASS. The independent `handoff-package-corrective-03.json` user-uploaded receipt matches archive size/hash/manifest identity.
- `manifest-corrective-03.json` SHA-256 `1a9e276fef99fd5d4401062fa2a40346bf933370a5c6826ccc8435510faf04ff`: **49/49 unique payload paths**, independently matched bytes and SHA-256; manifest itself excluded from self-hash.
- Source `run_d3_r3.py`: SHA-256 `e941b4c226993d545551ca0fafe411a02c01af95152ba94a6e83f4400bec2cd3`; original `run_d3.py` and `run_d3_r2.py` match prior independently reviewed originals. All AST-parsed successfully.
- `r2→r3` patch SHA-256 `157e31506853dc1a476c9cbaa4c26ba04ec837ebbb18fe08a77dfd999fae8845` and original→r3 comprehensive patch were **byte-exactly regenerated** from saved sources and match both patch files. The r2→r3 changes affect only `main()`'s physical root verification / attempt-03 source, invocation and output metadata; `case()`, `meta()`, fake budget/readiness cases remain unchanged from the pre-reviewed r2 source.
- Original nested `handoff.zip` SHA-256 `899d8899ada25c1774b2c4ad9b4c7db23a1df8fc5efc6c39fde4af3f2cb2973a` and `handoff-corrective-02.zip` SHA-256 `7eca88727d7259a41edf37d8d9a290e3ad87fcda764df3f7f3782d49c612d6d3` are byte-identical to independently received uploaded historical ZIP files. Nested CRC PASS and overlapping member bytes agree.

**Important producer count correction:** `preservation-before-03.json` and `preservation-receipt-03.json` list **47 rows, but only 36 distinct file paths**. **Eleven** paths each occur twice with the same bytes and SHA-256, so the phrase `47/47 original files unchanged` must be interpreted as **47/47 internally consistent receipt entries for 36 distinct paths**, not 47 unique originals. All 36 different paths byte/hash-match in the final archive. Independent 49-unique-payload manifest and two preserved historical ZIPs provide separate byte-level completeness evidence. This evidence variance is **explicitly retained**, not misreported as 47 distinct files or silently repaired in the sealed receipt.

## Fake-model test and negative-oracle findings

- `cases.json`: **53 unique fake-only case IDs, HTTP 26 + HTTPS 27**; all 53 recorded PASS with zero failures, matching expected named guard, invariant, ownership, and asserted text where applicable. These are archived **producer test records**, corroborated by static source logic, not an independent rerun of the Windows harness.
- **Actual zero/negative initial budgets**: four HTTP/HTTPS cases show initial budgets 0 and -1, no I/O, no accepted success, expected `INVALID_BUDGET` guard and fail-closed invariant. Pending/failed/ready `SO_ERROR` are three distinct **symbolic fake** branches, with different outcomes; none proves real plain TCP socket readiness.
- `run_d3_r3.py` uses the **same `case()` acceptance evaluator for ordinary and negative controls**. `proof_negative()` checks valid baseline before/after, named mutation failure, and observed fields; meta guards include false guard/invariant/owner, invalid baseline, silent acceptance, wrong named failure, and fake-as-SDK claim.
- **Eight** unique meta-control records PASS. A separate deliberately broken `always_pass_oracle` wraps the real `case()` and forcibly reports PASS; **six meta-controls then fail**, including all three required guard/invariant/ownership checks. Hence the saved `always_pass_oracle_meta_verdict=FAIL` reflects the intended rejection, not a pass of a broken acceptance oracle. This closes the prior M1 disconnected-meta-oracle defect **within model-only evidence**.
- `process-03.json` provides **producer-recorded** exit code 0, 141 ms and no timeout under a 120-second watchdog. Independently inspected actual `stdout-03.txt` is 248 bytes and `stderr-03.txt` **0 bytes**, matching exact recorded SHA-256/byte lengths. A matching `invocation-03.json` carries command, interpreter, CWD and process ID. The originating Windows process has **not** been directly observed by ChatGPT; `nested_process_attempts=0` is Python audit-hook/source evidence, not an OS-wide process trace.
- `preflight-03.json` and `invocation-03.json` repeat a **39-character truncated Playbook SHA** `29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70`. The archived `result.json` has the complete `29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70d`, also independently resolved against the pinned Playbook GitHub repository. The former originals are not altered. Historical original attempt-02 `__file__` value was never recorded; present r3 acceptance does not retrospectively recover it.

**Independent bounded audit:** **54/54** archive/source/JSON consistency checks PASS, *not 54 additional runtime tests*. Separate ChatGPT-generated audit report: `gatea-d3-attempt03-independent-audit-20261027.json`, SHA-256 `11057dcc1123be9778446faa848e08b25b1d6802b43b51187d788f0168afd0a3` (conversation download artifact, **not** committed to the repository). The audit file documents the preservation-count caveat explicitly.

## Final scope and preserved gates

- D3's **three permitted aggregate attempts are exhausted**: run01 archived fake-only PASS with inadequate original meta oracle; run02 exit 1 due pre-case root guard, preserved FAIL; run03 model-only verifier/cases PASS with connected negative oracle, independently archive-accepted. **NO attempt04 / re-execution / new root**.
- **Stage D3 result:** scoped D3 fake-model verification evidence accepted and child CLOSED. **Architecture/physical SDK feasibility remains `INCONCLUSIVE`**: real ESP-IDF aggregate deadline, DNS, TLS identity, event-loop callback quiescence, resource teardown / `D_physical` / `D_cleanup` not validated; prior G6 guard-table path discrepancy is still unresolved; no real C6/C3 SDK build/link, CI, real HTTP/socket/network or hardware test.
- Historical Stage 2R remains `STAGE2R_TEST_ONLY_CORRECTIVE_ACCEPTED__TRACE_LIMITED` CLOSED; original Stage 2 HTTPS map mismatch **UNRESOLVED**. **Gate A PENDING/UNPROVEN, Gate B HOLD, Stage 4 COLD, Library readiness PRESERVED.**
- **No subsequent executable child is admitted** by this archive review. Any further native-source evidence, SDK or architecture action requires independent scope selection and explicit authorization.
