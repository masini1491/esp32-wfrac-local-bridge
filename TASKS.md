# Unfinished work queue

## Hot — Add exact WF-RAC 0x13 EEV network pulse-value decoder

**Goal：** 在既有 operation-data classifier與 typed read-side decoder pattern之上，為 WF-RAC `0x13` 新增 software-only、read-only、raw-preserving 的 EEV network pulse/value decoder；只把 upstream 已建立的 `OP2` network value暴露成 typed exact byte value，不建立 calibrated valve opening、full-open scale或百分比。

**User-authorized scope（2026-10-06）：**
- 使用者已明確要求執行下一個 software-only bounded residual-diagnostics tranche。
- 授權只涵蓋 portable `wfrac_protocol` 的 `0x13` read-side typed decoder、host tests、必要 topic/spec/README/roadmap reconciliation與既有 compile-smoke/CI integration。
- 不授權 hardware/network、operation-data write、Device Model/Matter/runtime consumer integration、thermistor curve、vane/3D Auto或其他 residual diagnostic admission。

**Pinned evidence / evidence level：**
- Primary WF-RAC authority：`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
  - `docs/wf-rac-module-reference.md` §5.4 records `0x13` Outdoor EEV [pulses] with formula/value `OP2`, including observed network values such as 200 idle and 95/102 under load.
  - Multi-split evidence shows `0x13` differs per indoor unit and can be 0 on an idle indoor unit while another active unit has a non-zero value; project scope remains `per_indoor`.
  - The same reference explicitly says the full-open pulse count is unknown; direct CNS/SPI uses a 16-bit quantity while WF-RAC exposes a single-byte `OP2`, likely narrowed/derived rather than a raw pass-through of that counter.
  - Reference projects may derive `OP2 * 100 / 255` as a relative display, but the pinned documentation explicitly says this is **not calibrated valve opening** and should not be presented as physical percentage.
- Current project contract already classifies `0x13` as `CONFIRMED_QUERY_VALUE`, semantic `EEV-related raw/pulse value`, scope `per-indoor`, `calibrated_open_percentage=false`, local validation `HARDWARE_TEST_PENDING`.
- This Stage may call the returned byte exact **WF-RAC network value / pulse value**. It must not claim the value is the direct CNS 16-bit EEV counter, calibrated physical pulses, full-open fraction, or `CONFIRMED_LOCAL`.

**Scope / architecture：**
- Extend the existing `wfrac_protocol::OperationSegment` / operation-data owner; do not create a parallel parser.
- Add one typed read-side value type and decoder for `0x13`.
- Preserve the complete raw segment `[code, op1, op2, op3]`.
- Successful decode returns the exact `OP2` byte as the canonical network EEV pulse/value representation; no floating-point-only representation.
- Keep evidence/quality semantics visibly raw/network-level. Reuse `raw_only` if sufficient; do not relabel it canonical/calibrated physical truth merely because byte extraction is exact.
- Do not infer a universal selector from unrelated diagnostics. Current classifier treats `0x13` by code; if implementation introduces selector admission, it must be justified by pinned WF-RAC evidence rather than assuming universal `OP1=0x10`.
- `OP3` is preserved and has no admitted conversion role.
- No percentage helper, no `/255` opening fraction, no full-open constant, no inferred valve direction/state boolean.
- Do not inject the value into Device Model, Matter, RuntimeOrchestrator, persistence, availability or control logic.

**Residual boundaries：**
- `0x81/0x87` thermistor candidate curve remains pending.
- `0x82` remains raw-only with physical unit UNKNOWN.
- `0xB1` remains raw-only; Celsius conversion remains unestablished.
- `0x7C` remains UNKNOWN and must not become a protection boolean.
- `0x1E/0x1F` and unknown semantic codes retain current classification.
- Vane / horizontal / 3D Auto / advanced fixed bytes remain outside this Stage.

**Evidence/status reconciliation：**
- Update `docs/references/topics/operation-data.md` to distinguish:
  - canonical physical/software decoders (`0x80/0x94`);
  - inferred physical candidates (`0x11/0x90/0x85`);
  - exact raw/network typed EEV value (`0x13`);
  - remaining raw/UNKNOWN diagnostics.
- Update `specs/wf-rac-contracts.json` minimally to record the typed software decoder and preserve `calibrated_open_percentage=false` / `HARDWARE_TEST_PENDING`.
- README/roadmap only if current status or derived metrics actually change; do not inflate evidence level.

**Validation / completion gate：**
- Deterministic host tests must cover at least:
  - representative `OP2` values `0`, observed `57`, `95`, `102`, `200`, and `255`;
  - exact byte preservation with no rounding/scaling;
  - full raw preservation including `OP1` and `OP3`;
  - unsupported code and null-output behavior consistent with existing `OperationValueError` contract;
  - output reset / alias-safe semantics consistent with existing typed decoders;
  - regression proving no percentage/full-open/calibrated physical field is introduced;
  - existing `0x13` classifier remains `per_indoor` and raw/network quality does not become inferred/canonical physical truth.
- Existing host/protocol regression remains green.
- Materially affected C6/C3 ESP-IDF compile/link gates remain green; no firmware/network/hardware execution.
- Library readiness = `PRESERVED`.
- `git diff --check` PASS; README metrics, if affected, are recomputed from final tracked content.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back; final report states exact decoder semantics, evidence boundary, remaining residual diagnostics, validation evidence and result SHA.

**Hard exclusions：**
- No `OP2 / 255` percentage or full-open fraction.
- No calibrated physical-opening claim and no direct-CNS-counter equivalence claim.
- No operation-data write or exploratory control.
- No hardware/network validation.
- No Device Model / Matter / runtime consumer integration.
- Do not execute the hardware Cold Candidate.
