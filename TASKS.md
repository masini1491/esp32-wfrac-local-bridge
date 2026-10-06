# Unfinished work queue

## Hot — Add exact 0x82 THO-R1 network raw-value decoder

**Goal：** 為 WF-RAC operation-data `0x82`（THO-R1 outdoor heat-exchanger channel）新增 software-only、read-only、raw-preserving 的 typed decoder，只暴露 exact WF-RAC network OP2 byte與現有 shared-outdoor scope；physical unit維持 UNKNOWN，不建立任何溫度換算或 calibration claim。

**User-authorized scope（2026-10-06）：**
- 使用者已明確要求執行下一個 software-only residual。
- 授權僅涵蓋 portable `wfrac_protocol` 的 `0x82` typed raw decoder、host tests、必要 spec/topic/README/roadmap reconciliation與既有 compile-smoke/CI integration。
- 不授權 `0xB1`、`0x7C`、unknown-code semantics、vane/3D Auto/advanced fixed-state、Device Model/Matter/runtime consumer integration、hardware/network或任何 operation-data write。

**Pinned evidence / evidence level：**
- Primary WF-RAC authority：`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
  - `0x82` = THO-R1, outdoor heat-exchanger channel。
  - Observed raw values include idle `0x3f` (=63) and load `0x49` (=73); it tracks the shared outdoor unit.
  - Multi-split evidence shows `0x82` is shared across indoor units, unlike per-indoor `0x81/0x87`.
  - Upstream notes a possible different-resistor thermistor model (`Rs≈1230Ω`) but explicitly states there is no calibrated point yet and keeps `0x82` as a raw byte.
  - MHI-AC-Ctrl has no established THO-R1 formula; `0x82` must not reuse the `0x81/0x87` indoor-coil curve.
- Current project authority already classifies `0x82` as `shared_outdoor`, `raw_only`, `physical_unit=UNKNOWN`, `HARDWARE_TEST_PENDING`.
- This Stage must not promote evidence to `CONFIRMED_LOCAL` or a physical temperature relation.

**Scope / architecture：**
- Extend the existing operation-data owner; do not create a parallel parser.
- Add a typed read-side representation for `0x82` that preserves:
  - complete raw segment `[code, op1, op2, op3]`;
  - semantic identity THO-R1 / outdoor heat-exchanger raw channel;
  - shared-outdoor scope;
  - exact OP2 network byte as the typed raw value;
  - `raw_only` / UNKNOWN physical-unit semantics.
- OP1 and OP3 are preserved but have no admitted conversion role in this Stage.
- Do not impose a universal OP1 selector gate unless current project authority already requires one for `0x82`; otherwise accept by semantic code and preserve OP1.
- Do not interpret OP2 zero as temperature zero, sensor absence, compressor state, frost state, defrost state or no-value unless current evidence explicitly supports such semantics.
- Do not derive Celsius/deci-Celsius, resistance, thermistor curve, delta-T, frost/protection state or any secondary physical conclusion.
- Do not use the `0x81/0x87` R25/B/G/Rs curve, and do not adopt the upstream tentative `Rs≈1230Ω` as production conversion truth.
- Do not inject this value into Device Model, Matter, RuntimeOrchestrator, persistence, availability or control logic.

**Evidence/status reconciliation：**
- Update `docs/references/topics/operation-data.md` and, only if needed for clarity, `temperature-conversion.md` to record the exact typed raw boundary and explicitly preserve UNKNOWN physical unit.
- Update `specs/wf-rac-contracts.json` minimally to record typed `0x82` raw decoding, shared-outdoor scope, exact OP2 semantics, raw preservation, no indoor-curve reuse, physical unit UNKNOWN and `HARDWARE_TEST_PENDING`.
- README/roadmap only if current status or derived metrics materially change; do not inflate evidence level.

**Validation / completion gate：**
- Deterministic host tests must cover at least:
  - representative `0x82` raw values including 0, 63, 73, 255;
  - exact output byte equals OP2;
  - full raw preservation including arbitrary OP1/OP3;
  - shared-outdoor scope / semantic identity;
  - alias/reset behavior consistent with current `OperationValueError` pattern;
  - unsupported code and null output behavior;
  - regression that no Celsius/deci-Celsius or `0x81/0x87` curve is applied;
  - existing `0x81/0x87` behavior remains unchanged.
- Existing host/protocol regression remains green.
- Materially affected C6/C3 ESP-IDF compile/link gates remain green; no firmware/network/hardware execution.
- Library readiness = `PRESERVED`.
- `git diff --check` PASS; README physical-line metrics, if changed, are recomputed from final tracked content.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back; final report states raw-only/UNKNOWN-unit boundary, validation evidence, remaining residual diagnostics and result SHA.

**Hard exclusions：**
- No `0x82` temperature conversion.
- No adoption of tentative `Rs≈1230Ω` as production conversion.
- No reuse of `0x81/0x87` indoor-coil curve.
- No frost/defrost/protection/compressor-state inference.
- No `CONFIRMED_LOCAL` promotion.
- No Device Model / Matter / runtime consumer integration.
- No hardware/network validation.
- Do not execute Cold items.
