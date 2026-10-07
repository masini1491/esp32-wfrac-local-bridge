# Unfinished work queue

## Hot — Tighten 0x7C protection-diagnostic semantic guard

**Goal：** 對 WF-RAC operation-data `0x7C` 做 software-only semantic guard tightening：保留目前「known query code / observed response」事實，同時把其 scope、non-zero meaning、boolean/protection semantics 明確維持 UNKNOWN，避免把 raw diagnostic 誤建模成 compressor-protection-active 或 overload-state boolean。

**User-authorized scope（2026-10-07）：**
- 使用者已明確要求執行下一個 residual semantic-tightening tranche。
- 授權僅涵蓋 `0x7C` classifier/spec/docs/tests 的 bounded semantic guard 與必要 compile-smoke/CI regression。
- 不授權 `0x1E/0x1F`、`0x0D/0x21/0x32/0x34/0x35`、vane/3D Auto/advanced fixed-state、Device Model/Matter/runtime consumer integration、hardware/network 或任何 operation-data write。

**Pinned evidence / evidence level：**
- Primary WF-RAC authority：`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
- `0x7C` is queried as `Protection Number (raw)` and answers on three tested units across single- and multi-split setups.
- Observed value remained `0`; a controlled overload-clamp test also left `0x7C` unchanged while compressor frequency floor behavior changed.
- Upstream therefore distinguishes `0x7C` from general overload/protection-clamp activity; possible non-zero values may relate to protective stops/faults, but that mapping is not established for this project.
- Current project authority already keeps `0x7C` as `protection_diagnostic_unknown`, `scope=unknown`, `value_quality=unknown_semantic`, `CONFIRMED_UPSTREAM` + `HARDWARE_TEST_PENDING`.

**Scope / architecture：**
- Keep `0x7C` a known safe query code and preserve complete raw `[code, OP1, OP2, OP3]`.
- Keep semantic kind `protection_diagnostic_unknown`.
- Keep `scope=unknown`; do not infer per-indoor/shared-outdoor scope from current evidence.
- Keep `value_quality=unknown_semantic`; do not promote to `raw_only` with a stronger implied meaning, calibrated value, enum, boolean, or fault code mapping.
- Explicitly establish negative guards:
  - `0x7C == 0` does **not** mean protection inactive / false.
  - non-zero `0x7C` meaning remains UNKNOWN.
  - `0x7C` is **not** a general `compressorProtectionActive`, overload-clamp, speed-limit, frost/defrost, or generic fault boolean.
  - observed overload clamp with `0x7C=0` must remain compatible with the model.
- Do not add a physical/unit conversion.
- Do not add Device Model, Matter, runtime, persistence, availability, alarm/fault, or control integration.

**Evidence/status reconciliation：**
- Update `docs/references/topics/operation-data.md` minimally to record the observed-response / negative-semantic boundary.
- Update `specs/wf-rac-contracts.json` minimally so `0x7C` explicitly preserves:
  - `CONFIRMED_QUERY_VALUE` / known query presence;
  - full raw preservation;
  - scope UNKNOWN;
  - semantic/value meaning UNKNOWN;
  - not a general protection-active/overload boolean;
  - non-zero mapping unestablished;
  - `HARDWARE_TEST_PENDING`.
- README/roadmap only if materially necessary; do not inflate status/evidence.

**Validation / completion gate：**
- Deterministic host tests must cover at least:
  - representative `0x7C` raw segments with OP2=0 and non-zero OP2;
  - exact raw preservation including arbitrary OP1/OP3;
  - classifier remains `protection_diagnostic_unknown`;
  - scope remains `unknown`;
  - quality remains `unknown_semantic`;
  - `code_known=true`, `unit_relation_known=false`;
  - existing safe-query inclusion remains unchanged;
  - no boolean/enum/physical conversion API is introduced;
  - nearby `0xB1`, `0x1E/0x1F`, and unknown-code classifications remain unchanged.
- Existing host/protocol regression remains green.
- Materially affected C6/C3 ESP-IDF compile/link gates remain green; no firmware/network/hardware execution.
- `git diff --check` PASS.
- Library readiness = `PRESERVED`.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back; final report states the negative semantic guard and that hardware/network validation was not performed.

**Hard exclusions：**
- No `0x7C` protection boolean.
- No non-zero fault-code mapping.
- No scope inference.
- No `CONFIRMED_LOCAL` promotion.
- No Device Model / Matter / runtime consumer integration.
- No hardware/network validation.
- Do not execute Cold items.
