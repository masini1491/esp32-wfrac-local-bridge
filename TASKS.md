# Unfinished work queue

## Hot — Operation-data 0x13 EEV idle-context evidence guards

**Authorization / identity (2026-10-08):** User approved execution of the bounded software-only Stage after Cold admission review. This is the only current Hot, not a launch of other software candidates or hardware. Start reference `masini1491/esp32-wfrac-local-bridge@main` `84943760909c84d8151b5fcad0c926f4f21055bc`; shared Playbook `masini1491/ai-development-playbook@main` `65b6f1546cf3412aaa79ee272a752cdb4ca48f45`. Fresh-resolve before execution; handoff revisions are historical identity, not current authority.

**Goal:** Incorporate newly pinned upstream *user-reported* multi-split `0x13` EEV idle readings as bounded provenance and negative semantic guards. No decoder/command/runtime/Matter feature work. Maintain raw per-indoor OP2 observation with physical units and calibrated opening UNKNOWN.

**Evidence / provenance:**
- Existing primary protocol pin: `blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`, scoped current `docs/references/topics/operation-data.md`, `docs/references/sources/mitsubishi-wf-rac-integration.md`, `specs/wf-rac-contracts.json` `operation_data_r3`, and `decode_eev_network_value()`.
- Supplemental **exact** upstream commit `blues-sechseck/Mitsubishi-WF-RAC-Integration@966cedaa731b4d28b648f6ab58a1b0a16714c3e3` (merged PR #399, 2026-10-05), sole source change in `docs/wf-rac-module-reference.md` §5.4, citing discussion #328. Its newly described four-indoor multi-split measurements are labeled `[UR]` (user report), NOT a universal firmware guarantee or local project measurement.
- In that reported installation, no-demand indoor EEV `0x13` did not always read zero: with sibling cooling, it could read zero; with sibling heating, it remained nonzero; all units idle could report `0xC8`. The cited 16–24% *of 0xC8* is a **relative raw observation**, not calibrated valve opening/percentage or an adopted conversion. Do not collapse differing `[HW]` and `[UR]` evidence roles.
- Existing code already preserves `[code,op1,op2,op3]` and exposes `decode_eev_network_value()` raw OP2. No source defect or verified actual actuation is established; existing local validation remains `HARDWARE_TEST_PENDING`.

**Authorized Codex mutation scope** (only listed files; `TASKS.md` closure permitted):
1. `docs/references/topics/operation-data.md`: add narrow factual and negative-guard note on system operating direction, unit demand and varying idle raw values; cite exact supplemental commit and discussion; distinguish report-specific observations from universal rules.
2. `docs/references/sources/mitsubishi-wf-rac-integration.md`: add a concise separately attributed supplemental revision/pointer without replacing the existing primary pin or globally upgrading source authority.
3. `specs/wf-rac-contracts.json`: add minimum machine-readable, typed *negative* guards for `operation_data_r3` code `0x13` (prefer its existing entry/owner). Forbid `no_indoor_demand ⇒ OP2=0`, `OP2=0 ⇒ universal valve closed`, `OP2>0 ⇒ demand`, universal idle value, model/firmware-wide rule, calibrated percentage/full-open inference; mark per-indoor scope, raw-preserved, hardware pending, provenance as supplemental user report where needed. Do **not** treat any reported values or ratios as generic physical constants.
4. `tools/validate_contracts.py`: validate these new exact fail-closed guards and section types, including no accidental evidence level promotion, without relaxing old `0x13` invariants.
5. `tests/host/test_validate_contracts.py`: add targeted canonical-pass and mutation negative tests that exercise `validate()`; reject changing each safety-relevant guard, missing/wrong-type fields, universal no-demand/idle/zero claims or physical/calibrated/write projection.
6. `TASKS.md`: only coordination and final closure to minimal empty queue, after gates pass.

**Hard exclusions:** Do not modify `components/` source, `decode_eev_network_value()`, Device Model, Matter adapter, API, runtime, command/query behavior, firmware, workflow, test C++ source, README or unrelated files. No magic 0x13 threshold, no `0xC8` universal constant, no applying 16–24% as calibration, no automated valve-open/closed, demand, occupancy, heating/cooling or feature-support inference. Do not revise the global primary upstream pin merely because its latest main advanced. Do not start adjacent Cold or first-real-probe work.

**Validation / completion:**
- Python syntax `python -m py_compile tools/validate_contracts.py tests/host/test_validate_contracts.py`; `python tools/validate_contracts.py`; `python -m unittest tests.host.test_validate_contracts`; negative mutations and `git diff --check` must PASS.
- Require exact-candidate GitHub Actions `Validate WF-RAC contracts` including Python + host C++ regressions PASS; report tested SHA and run/job. Documentation/Python-only changes do not require a new ESP32-C6/C3 compile/link gate; do not claim a queued/skipped build as PASS.
- Only files above changed. Preserve `Library readiness: PRESERVED`; `CONFIRMED_LOCAL=0`; local/hardware/network remains `HARDWARE_TEST_PENDING`. No user credentials, logs, network/WF-RAC probes or real firmware runtime.
- Codex uses current `AGENTS.md` + Playbook safe-sync, clean working tree, non-force Git route. Preserve the old seven-file dirty vertical-vane workspace; use a separate clean sibling clone if needed. If source evidence clashes or permissions/build gates cannot be closed, STOP with evidence and keep Hot open rather than guessing.
- Only on all gates PASS: integrate via legal Git route, close `TASKS.md`, canonical remote read-back and report execution revisions, actual validations, Child delegation/profile override, Library readiness.
