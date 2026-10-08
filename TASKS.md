# Unfinished work queue

## Hot — Docs reconciliation

**Goal:** Reconcile stale README / roadmap status against current canonical source, tests and CI without changing technical scope.

**Authorized scope (2026-10-08):**
- Documentation/bookkeeping only.
- Primary targets: `README.md` and `docs/roadmap.md`.
- Reconcile current validator test count and stale phase/status wording that contradicts already-completed canonical implementation.
- Do not modify production source, tests, specs, workflows, architecture, protocol semantics, security policy, hardware/network behavior, or future-stage scope.

**Known current drift to reconcile:**
- README and roadmap still state 66 Python validator tests; current completed Stage evidence is 69 validator tests.
- Phase 1A roadmap still groups several already-completed areas under “尚未實作”, including request RECEIVE/composition-related work, diagnostic software boundaries, Device Model foundations, JSON/API/transport/runtime foundations now documented elsewhere as completed.
- Phase 1B roadmap similarly contains an over-broad “尚未實作” list that includes completed Base64/API envelope, queue/orchestration, retry, timeout/polling/availability foundations.
- Preserve unresolved/hardware-dependent items as pending; do not overstate completion.

**Reconciliation rules:**
- Current source/tests/CI and canonical topic docs outrank stale README/roadmap summaries.
- Prefer precise split wording: completed software-only foundation vs still-pending hardware/runtime/consumer integration.
- Keep `CONFIRMED_LOCAL`, `HARDWARE_TEST_PENDING`, compile-ready/runtime-not-executed boundaries unchanged.
- Do not alter project phase or introduce a new roadmap phase.

**Validation / completion gate:**
- Only intended docs change.
- No source/test/spec/workflow diff.
- README and roadmap no longer claim 66 validator tests.
- No completed capability remains falsely listed as wholly “尚未實作” where current canonical docs/source show a completed software foundation.
- No pending hardware/network/runtime capability is falsely promoted to completed.
- `git diff --check` PASS.
- Library readiness = `PRESERVED`.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back.

**Hard exclusions:**
- No implementation.
- No protocol or architecture redesign.
- No test changes merely to match documentation.
- No hardware/network/Matter runtime execution.
- Do not execute the next Cold stage automatically.
