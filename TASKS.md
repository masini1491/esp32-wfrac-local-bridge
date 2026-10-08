# Unfinished work queue

## Hot — Extended observation validator guards and documentation reconciliation

**Authorization (2026-10-08):** User approved execution of the two issues found during the completed PR #3 audit. This is one bounded software-only maintenance Stage; no other Cold candidate or hardware work is admitted.

**Canonical start:** `masini1491/esp32-wfrac-local-bridge@main` `6e9e0542a62cf9af02596ade25899376441853d2`; shared `masini1491/ai-development-playbook@main` `65b6f1546cf3412aaa79ee272a752cdb4ca48f45`. Fresh-check again before execution; these SHAs are handoff reference, not substitute for current authority. Project `AGENTS.md` and current source/tests own implementation truth.

**Problem / goals:**
1. `specs/wf-rac-contracts.json` has `device_model_extended_observation`, but `tools/validate_contracts.py::validate()` does not require or enforce that contract object; existing `tests/host/test_validate_contracts.py` tests only affirm a subset of the canonical contents. This allows regression in fail-closed input, field list, hardware/local status, or forbidden Matter/control claims to evade the current CI contract gate.
2. `README.md` and `docs/roadmap.md` include stale `69` Python-test counts (exact-main host CI ran 72 before adding these tests), and current six-cell ESP32-C6/C3 workflow is described as the original four-cell gate. Historical four-cell workflow runs remain valid historical evidence, not current coverage.

**Authorized modification scope (only four implementation/docs files, plus final TASKS closure):**
- `tools/validate_contracts.py`
- `tests/host/test_validate_contracts.py`
- `README.md`
- `docs/roadmap.md`
- `TASKS.md` for queue closure only.
- Do not modify canonical contract values/shape, production C++ or headers, ESP-IDF firmware, workflows, dependency pins, architecture/security, or other files. On genuine contract contradiction, STOP and report it rather than silently changing source truth.

**Implementation contract:**
- Admit `device_model_extended_observation` as a required top-level section; validate that it is a correctly typed object with the exact admitted `implemented=true`, `input="structurally_valid_dual_crc_AirconFrame_RECEIVE_only"`, the existing complete ordered set of raw/typed `fields`, `capability_hints_are_separate=true`, `mutates_basic_hvac_state_or_device_state=false`, `infers_physical_actuation_or_occupancy=false`, `matter_projection="unsupported"`, `adds_matter_attributes_or_clusters=false`, `adds_writable_intents=false`, `local_validation_status="HARDWARE_TEST_PENDING"`.
- Use deterministic exact-value/type guards, not truthiness checks that accept strings, integers or missing/extra semantics. Ensure absent/wrong-type section gives controlled `ContractError` instead of silent acceptance or uncaught `AttributeError`; preserve existing validator error conventions. Do not invent additional meaning or control.
- Add focused mutation negative tests for omitted section, wrong input/fields (missing/reordered/added), mis-typed fields, every safety-relevant false/true guard inversion, Matter promotion, writable intent, physical inference, and local evidence elevation. Confirm canonical contract still passes. Tests must exercise the production `validate()` entrypoint; don't only assert hardcoded fixtures.

**Documentation reconciliation:**
- Replace brittle *current test totals* in README/roadmap with a durable description of the Python `unittest` host gate and exact CI provenance (optionally note a dated snapshot only if clearly labeled historical). Do not hard-code `72` as an evergreen current total, because this Stage adds new tests.
- Clarify that the current `.github/workflows/esp-idf-compile.yml` has six C6/C3 compile/link jobs: C6+C3 × `idf_compile_smoke`, `wfrac_bootstrap_probe/default`, `wfrac_bootstrap_probe/webui`. Historical four-cell runs and original four default cells can remain identified as historical/subset. Do not imply firmware execution or hardware/network validation.
- Keep `README.md`, `docs/roadmap.md`, and pre-existing safety/evidence boundaries consistent. Avoid unrelated content edits or rewriting historical provenance.

**Validation / completion gate:**
- `python -m py_compile tools/validate_contracts.py tests/host/test_validate_contracts.py`
- `python tools/validate_contracts.py`
- `python -m unittest tests.host.test_validate_contracts` and mutation negative tests PASS, report exact new test total and candidate SHA.
- Applicable existing host C++ regression / GitHub Actions `Validate WF-RAC contracts` workflow PASS for exact candidate; Python/doc-only modification does not materially require a new C6/C3 build. If the usual main-push ESP-IDF CI runs, report it accurately; never call skipped/queued jobs PASS.
- `git diff --check` PASS; changed files restricted to authorized paths. Library readiness `PRESERVED`.
- No actual firmware/network/hardware, Matter SDK/runtime, NVS, credentials, or `CONFIRMED_LOCAL` promotion.
- On all gates PASS, complete via project-approved Codex Git route, close `TASKS.md` to minimal empty Hot queue (heading-only or established EMPTY convention), commit/push and canonical remote read-back. If blocked, preserve candidate and leave Hot open with exact blocker.

**STOP boundaries:** No protocol/Device Model/Matter implementation, no new features or hardware assumptions, no adjacent future Cold work, no stale dirty-worktree overwrite. Respect prior preserved dirty workspace; if currently in it, use fresh authorized clean sibling clone/worktree. Do not modify user system/toolchain settings merely to obtain local tests; use existing remote host workflow where appropriate.
