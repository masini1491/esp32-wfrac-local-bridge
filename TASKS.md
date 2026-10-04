# Unfinished work queue

## Hot — Reconcile TimeProvider machine-readable implementation state

**Goal：** 將 `time_architecture_r5` 的 machine-readable state 與 current portable `TimeProvider` / ESP-IDF `EspSystemTimeProvider` implementation 對齊：明確表達 provider boundary、quality enum 與 explicit seed/adopt system-time adapter 已實作，同時保留 trusted/estimated time acquisition、persistence、power-loss recovery、SNTP/NTP 與 production runtime 尚未完成。

**Current evidence / authority：**
- Portable `wfrac_api::ITimeProvider`、`TimeSample` 與 `TimeQuality { unavailable, estimated, trusted }` 已存在。
- `wfrac_time_espidf::EspSystemTimeProvider` 已實作：
  - `gettimeofday()` read；
  - explicit `seed(TimeSample)` via `settimeofday()`；
  - explicit `adopt_current_system_time(TimeQuality)`；
  - initial / invalidated quality = unavailable。
- Current architecture / reference topic / Phase 1C roadmap already describe this implementation as complete at the provider/adapter boundary。
- Current machine-readable `time_architecture_r5` still declares `future_time_provider.exact_runtime_enum = PENDING` and `implementation = false`, and validator/tests enforce that stale state。
- Overall `time_architecture_r5.runtime_implemented = false` remains correct for production runtime; do not collapse provider-boundary implementation into full runtime completion。

**Required reconciliation：**
- Update `specs/wf-rac-contracts.json` so the time architecture can represent, without ambiguity:
  - portable TimeProvider boundary implemented；
  - exact current quality states implemented as `unavailable / estimated / trusted`；
  - ESP-IDF POSIX system-time adapter / explicit seed-adopt boundary implemented；
  - time acquisition source still pending；
  - persistence / power-loss recovery still pending；
  - SNTP/NTP not required and not implemented as daily-control dependency；
  - production runtime remains not implemented。
- Prefer the smallest vocabulary/schema evolution that preserves existing R5 semantics; do not rename/restructure unrelated contract sections merely for aesthetics。
- Update `tools/validate_contracts.py` and `tests/host/test_validate_contracts.py` so deterministic validation enforces the new implemented-vs-pending boundaries and still rejects false claims such as acquisition/persistence/runtime being complete。
- Reconcile `docs/references/topics/timestamp-time.md` stale wording, especially any statement that the exact time-quality enum is still unknown/pending。
- Reconcile `docs/roadmap.md` only where older Phase 1B wording still implies TimeProvider/timestamp itself is wholly unimplemented; distinguish completed provider/API boundary from still-pending acquisition/runtime integration。
- `docs/architecture.md` and README already substantially reflect the implemented boundary; change them only if current reconciliation reveals a concrete inconsistency. If deterministic test-count or tracked README metrics materially change, update only the affected derived facts rather than broad README prose。

**Hard exclusions：**
- No changes to `components/**`, firmware behavior, runtime logic, network behavior, SNTP/NTP, Wi-Fi, NVS, RTC selection, Matter, Operator ID, transport, protocol semantics, or hardware behavior。
- Do not claim trusted/estimated time acquisition exists merely because the provider accepts an explicitly supplied quality。
- Do not claim persistence or exact wall-time recovery across power loss。
- Do not change WF-RAC timestamp security/freshness/replay unknowns or expires/timezone unknowns beyond what current evidence supports。
- No hardware/network validation is required or authorized by this Stage。
- Do not execute either remaining Cold Candidate。

**Expected writable implementation surfaces：**
- `specs/wf-rac-contracts.json`
- `tools/validate_contracts.py`
- `tests/host/test_validate_contracts.py`
- `docs/references/topics/timestamp-time.md`
- `docs/roadmap.md`
- `README.md` only if an exact derived test/metric fact becomes stale
- `docs/architecture.md` only if a concrete inconsistency is found
- `TASKS.md` for completion bookkeeping

**Validation / completion gate：**
- Contract expresses provider/quality/ESP-IDF adapter implemented while acquisition/persistence/runtime remain pending。
- Validator has positive invariants for the implemented boundaries and negative guards preventing false promotion of acquisition/persistence/runtime。
- Targeted host validator tests PASS; run the existing contract validator / host test commands required by current repo authority。
- Documentation and machine-readable contract use consistent implemented-vs-pending semantics。
- No source/runtime files changed。
- `git diff --check` PASS and scoped diff contains only the expected reconciliation/docs/test surfaces。
- On success, remove this Hot Stage, return `TASKS.md` to minimal `EMPTY`, commit/push, and report exact validation totals, changed-file list, applicable Playbook revision, and final remote HEAD。
