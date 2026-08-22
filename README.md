# esp32-wfrac-local-bridge

這是將 Mitsubishi Heavy Industries WF-RAC 空調銜接至 Matter 的本地研究與架構基準。

研究 reference set 已補強，並記錄 AirconStat 與 MHI CNS/SPI-derived semantics 的高階關係；本專案仍處於 Phase 0，韌體與實機驗證目前暫緩。

Phase 0 的 machine-readable software foundation 已建立於 [`specs/wf-rac-contracts.json`](specs/wf-rac-contracts.json)，僅整理研究 contract，不包含 runtime implementation。

目前 software-only validation 流程為：`specs/wf-rac-contracts.json` → `tools/validate_contracts.py` → `tests/host/test_validate_contracts.py` → GitHub Actions。dependency-free host validator 與 12 個 targeted host tests 已通過，GitHub Actions software validation gate 亦已通過；`CONFIRMED_LOCAL = 0`，hardware/runtime validation 仍為 `HARDWARE_TEST_PENDING`。

最小 host validation commands：

```text
python -m py_compile tools/validate_contracts.py tests/host/test_validate_contracts.py
python tools/validate_contracts.py
python -m unittest tests.host.test_validate_contracts
```

## 專案狀態

目前是 **Phase 0 — Research / Architecture / Project Setup**。參考資料研究基準已建立；本 repository 尚未包含 firmware，韌體開發與實機驗證目前暫緩。

- Framework：ESP-IDF
- Primary MCU：ESP32-C6
- Compatibility MCU：ESP32-C3
- Primary transport：Matter over Wi-Fi
- WF-RAC：本地 HTTP/HTTPS API（Transport 尚未實作）
- 控制原則：Local-first；日常控制不依賴雲端、MQTT 或 Internet

架構方向為 `WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi`。Matter Adapter 只使用高階 Device Model request，不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。

## 範圍與證據

本階段只記錄研究、架構、硬體策略、安全與 roadmap。Reference facts 依 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING` 分級。WF-RAC 使用者硬體的 protocol/runtime validation 目前為 `HARDWARE_TEST_PENDING`。

## License

TBD。正式公開前再決定 project license；本研究基準不新增 LICENSE。

詳見 [docs/roadmap.md](docs/roadmap.md)。
