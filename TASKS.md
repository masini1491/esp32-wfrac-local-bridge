# Unfinished work queue

## Hot — Add exact 0xB1 TDSH-related network raw-value decoder

**Goal：** 為 WF-RAC operation-data `0xB1`（TDSH / discharge-superheat-related diagnostic channel）新增 software-only、read-only、raw-preserving 的 typed decoder，只暴露 exact WF-RAC network OP2 byte與 shared-outdoor scope；physical unit維持 UNKNOWN，不建立 Celsius conversion。

**User-authorized scope（2026-10-07）：**
- 使用者已明確要求執行下一個 software-only residual。
- 授權僅涵蓋 portable `wfrac_protocol` 的 `0xB1` typed raw decoder、host tests、必要 spec/topic/README/roadmap reconciliation與既有 compile-smoke/CI integration。
- 不授權 `0x7C`、`0x1E/0x1F`、unknown-code semantics、vane/3D Auto/advanced fixed-state、Device Model/Matter/runtime consumer integration、hardware/network或任何 operation-data write。

**Pinned evidence / evidence level：**
- Primary WF-RAC authority：`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
  - `0xB1` is identified as TDSH-related in the WF-RAC operation-data matrix.
  - Upstream lists an `OP2 / 2` Celsius interpretation only as inferred/cross-reference context; the two observed operating points both returned raw `0x00` and did not move, so they do not establish a calibrated or validated network Celsius relation.
  - Current project authority therefore keeps `0xB1` as raw-only with Celsius conversion unestablished.
- Current project authority: raw preserved, physical unit UNKNOWN, local validation `HARDWARE_TEST_PENDING`.
- This Stage must not promote evidence to `CONFIRMED_LOCAL` or a physical temperature relation.

**Scope / architecture：**
- Extend the existing operation-data owner; do not create a parallel parser.
- Add a typed read-side representation for `0xB1` that preserves:
  - complete raw segment `[code, op1, op2, op3]`;
  - semantic identity TDSH-related diagnostic raw channel;
  - shared-outdoor scope;
  - exact OP2 network byte as typed raw value;
  - `raw_only` / UNKNOWN physical-unit semantics.
- OP1 and OP3 are preserved but have no admitted conversion role in this Stage.
- Do not impose a universal OP1 selector gate unless current project authority explicitly requires one for `0xB1`; otherwise accept by semantic code and preserve OP1.
- OP2=0 remains an ordinary raw byte. Do not interpret it as 0°C, no-value, inactive superheat, compressor state, fault, or unsupported.
- Do not derive Celsius/deci-Celsius, superheat, discharge temperature, delta-T, protection/fault state, compressor state, efficiency or other secondary physical conclusions.
- **Do not implement `OP2 / 2` as production Celsius conversion.**
- Do not inject this value into Device Model, Matter, RuntimeOrchestrator, persistence, availability or control logic.

**Evidence/status reconciliation：**
- Update `docs/references/topics/operation-data.md` and, only if needed for clarity, `temperature-conversion.md` to record the typed raw decoder and explicitly preserve UNKNOWN physical unit / no admitted Celsius conversion.
- Update `specs/wf-rac-contracts.json` minimally to record typed `0xB1` raw decoding, shared-outdoor scope, exact OP2 semantics, raw preservation, Celsius conversion unestablished and `HARDWARE_TEST_PENDING`.
- README/roadmap only if current status or derived metrics materially change; do not inflate evidence level.

**Validation / completion gate：**
- Deterministic host tests must cover at least:
  - representative `0xB1` raw values including 0, 1, 127, 255;
  - exact output byte equals OP2;
  - full raw preservation including arbitrary OP1/OP3;
  - shared-outdoor scope / semantic identity;
  - OP2=0 remains a valid raw byte with no special physical meaning;
  - alias/reset behavior consistent with current `OperationValueError` pattern;
  - unsupported code and null output behavior;
  - regression that no Celsius/deci-Celsius or `OP2/2` conversion is exposed;
  - existing `0x82`, `0x81/0x87`, `0x85` behavior remains unchanged.
- Existing host/protocol regression remains green.
- Materially affected C6/C3 ESP-IDF compile/link gates remain green; no firmware/network/hardware execution.
- Library readiness = `PRESERVED`.
- `git diff --check` PASS; README physical-line metrics, if changed, are recomputed from final tracked content.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back; final report states raw-only/UNKNOWN-unit boundary, rejected Celsius inference, validation evidence, remaining residual diagnostics and result SHA.

**Hard exclusions：**
- No `0xB1` Celsius conversion.
- No production use of `OP2 / 2`.
- No superheat/discharge-temperature/protection/fault inference.
- No `CONFIRMED_LOCAL` promotion.
- No Device Model / Matter / runtime consumer integration.
- No hardware/network validation.
- Do not execute Cold items.
