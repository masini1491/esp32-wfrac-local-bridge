# WF-RAC protocol research index

本檔是已完成 Phase 0 research 的 high-level legacy/index 入口，不是未來 Codex task 的預設完整閱讀資料來源。日常閱讀請先看 [Reference index](references/README.md)，再只讀與任務直接相關的 topic。

## Evidence status

使用 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING`。目前 `CONFIRMED_LOCAL = 0`；upstream research 不是 hardware validation，使用者 WF-RAC protocol/runtime validation 仍為 `HARDWARE_TEST_PENDING`。Machine-readable contract 為 [specs/wf-rac-contracts.json](../specs/wf-rac-contracts.json)。

## Topic routing

- API、handlers、discovery：[api-envelope-discovery.md](references/topics/api-envelope-discovery.md)
- Identity、Operator、registration：[identity-registration.md](references/topics/identity-registration.md)
- AirconStat fixed state、set-bits：[airconstat-fixed-state.md](references/topics/airconstat-fixed-state.md)
- Operation-data matrix：[operation-data.md](references/topics/operation-data.md)
- ModelNrRaw、optional capability：[capability-model.md](references/topics/capability-model.md)
- timestamp、module clock、expires：[timestamp-time.md](references/topics/timestamp-time.md)
- temperature conversion：[temperature-conversion.md](references/topics/temperature-conversion.md)
- HTTP／HTTPS／TLS：[tls-transport.md](references/topics/tls-transport.md)

## Research status

- R1 identity／registration：已整理，local validation pending。
- R2 fixed 18-byte AirconStat state／command：已整理，codec 未實作。
- R3 operation-data WF-RAC-visible matrix：已整理，query與所有 field local validation pending。
- R4 capability／model detection：已整理，unknown model與optional features保守 pending。
- R5 Timestamp / Time Architecture：已整理，local validation 與 exact time-source runtime 仍 pending。
- R6 Temperature Conversion / Sensor Semantics：已整理；local hardware calibration remains pending。
- R7 TLS / Transport Security Policy：software-only research COMPLETE；device-specific TLS/certificate validation remains HARDWARE_TEST_PENDING。
- Matter mapping／Phase 0.3：尚未開始。

Source provenance 只在需要時讀 [sources/](references/sources/)；本 index 不重複完整 source history、fixed-byte table、operation-data matrix 或 capability flags。
