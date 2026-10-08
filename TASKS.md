# Unfinished work queue

## Hot — Horizontal vane / 3D Auto read-side

**Goal:** Implement bounded portable RECEIVE-side typed observation for horizontal left/right vane and Entrust/3D Auto using pinned WF-RAC evidence, with raw preservation and conservative UNKNOWN handling. This Stage is observation-only; no control/write admission.

**Authorization / sequencing (2026-10-08):** User authorized ordered execution of the software-only continuation sequence. Docs reconciliation and Vertical vane protocol core have passed canonical completion. This is the sole current Hot; advanced fixed-state guards and Device Model/Matter mapping remain Cold.

**Pinned evidence / authority:**
- Project current `AGENTS.md`, `docs/references/README.md`, `docs/references/topics/airconstat-fixed-state.md`, `docs/references/topics/capability-model.md`, machine spec and current protocol source/tests.
- Primary upstream `blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`, `docs/wf-rac-module-reference.md` §4.1–4.2 and `custom_components/mitsubishi_wf_rac/wfrac/rac_parser.py` RECEIVE decode. These are WF-RAC network RECEIVE bytes, not automatically direct CNS physical status or a universal model capability.
- `state[11] & 0x1F`: LR fixed position index raw 0..6 maps to positions 1..7 where swing does not override; preserve full byte and low-five raw.
- `state[12] & 0x03 == 0x01`: observed LR swing mode, ignoring the position byte for effective position.
- `state[12] & 0x0C == 0x04`: Entrust/3D Auto observed active bit pattern. Other values in this mask must not be silently treated as proof of disabled functionality.
- The upstream full-state COMMAND horizontal/entrust masks exist but are excluded from this Stage. Do not infer a safe project single-intent write path.
- Existing capability flags `wind_direction_lr` and `entrust` are app hints only; preserve profile-known vs unknown and do not equate hint with module wire observation or verified physical feature.

**Bounded implementation:**
1. Extend portable `wfrac_protocol` RECEIVE observation with distinct typed horizontal swing/fixed/unknown and Entrust active/inactive/unknown states, while preserving raw `state[11]` and `state[12]`; never convert malformed/reserved patterns to a valid fixed position, false/off, or invented default.
2. Represent LR fixed positions 1..7 only for proven raw low-five 0..6 with a non-swing state; swing overrides position. Retain evidence of unknown bytes, reserved combinations, and future model/firmware differences.
3. Keep CapabilityProfile/hints separate from observation and actual actuator support. Document `CONFIRMED_UPSTREAM` software decode, model capability unresolved, and `HARDWARE_TEST_PENDING`.
4. Update scoped fixed-state topic / machine-readable contract and deterministic host regression. Do not change Device Model state, `CommandIntent`, protocol command encoder, request composition, runtime, Matter, API, network or hardware.
5. Keep portable core free of ESP-IDF/FreeRTOS/Wi-Fi/NVS/Matter dependencies. Do not copy upstream implementation mechanically.

**Validation / closure:**
- Host C++ cases cover LR fixed 1..7, swinging with nonzero position bytes, invalid low-five 7..31, unknown/reserved `state[12]` low-two and Entrust mask encodings, raw-byte preservation, and independence from vertical-vane / basic HVAC decode.
- Explicit negative tests: no accidental `CommandIntent` or write support; capability hint does not enable or invent local feature.
- Python syntax, contract validator and 69 existing validator tests; C++ protocol and Device Model host regression; `git diff --check` PASS. Update targeted validator tests only if contract schema change materially requires them.
- For production `components/` changes run ESP32-C6/C3 compile/link in existing GitHub Actions isolated-candidate branch route if Windows local ESP-IDF remains blocked. Verify exact candidate SHA, six job outcomes and modified source participation. Staging candidate commit is not Stage closure. No workflow mutation solely to get CI.
- Library readiness `PRESERVED`; no hardware/network/physical validation; `CONFIRMED_LOCAL` not promoted.
- When all gates PASS, commit/promote through legal non-force route, close TASKS to minimal `EMPTY`, and perform remote canonical read-back. On blocker preserve candidate, record exact phase and STOP.

**Hard exclusions:** No horizontal/3D Auto COMMAND encoders, control/request write, model capability claims, Device Model/Matter/consumer integration, advanced fixed-state neighboring semantics, self-clean control, ESP-IDF hardware execution, Wi-Fi/TLS/WF-RAC traffic, or adjacent Cold stage.
