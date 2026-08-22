# esp32-wfrac-local-bridge

這是將 Mitsubishi Heavy Industries WF-RAC 空調銜接至 Matter 的本地研究與架構基準。

研究 reference set 已補強，並記錄 AirconStat 與 MHI CNS/SPI-derived semantics 的高階關係；本專案已進入 Phase 1C WF-RAC Transport Abstraction，實機驗證目前暫緩。

Phase 0 的 machine-readable software foundation 已建立於 [`specs/wf-rac-contracts.json`](specs/wf-rac-contracts.json)，僅整理研究 contract，不包含 runtime implementation。

目前 software-only validation 流程為：`specs/wf-rac-contracts.json` → `tools/validate_contracts.py` → `tests/host/test_validate_contracts.py`，並新增 `components/wfrac_protocol/` 的 C++17 CRC16／AirconStat structural framing、basic fixed-state decoder、四個 safe command intents、capability profile decoder、operation-data raw segment parser/classifier、Aircon block serializer、safe query builder、standard Base64 codec 與 generic full AirconStat composer host tests，皆由 GitHub Actions 執行。Python host tests 目前為 64 個；C++ protocol core tests 為 1 個 executable；`CONFIRMED_LOCAL = 0`，hardware/runtime validation 仍為 `HARDWARE_TEST_PENDING`。

最小 host validation commands：

```text
python -m py_compile tools/validate_contracts.py tests/host/test_validate_contracts.py
python tools/validate_contracts.py
python -m unittest tests.host.test_validate_contracts
```

## 專案狀態

目前是 **Phase 1C — WF-RAC Transport Abstraction**。Phase 1A Protocol Core #1–#6 與 Phase 1B Device Model foundations 已完成；目前另有 portable transport contract、endpoint/request/response DTO、TransportError 與 HTTP status 的明確分離、test-only FakeTransport、portable TimeProvider，以及 typed WF-RAC API request builders（JSON escaping、64-bit timestamp、AirconStat CRC validation 與 Base64 wrapping）。platform clock source、registration builders、Operator ID persistence、HTTPS-first scheme state machine、per-device scheme cache、ESP-IDF HTTP/TLS transport、mDNS、retry/rate limiting、NVS、Matter mapping 與實機驗證仍暫緩。

- Framework：ESP-IDF
- Primary MCU：ESP32-C6
- Compatibility MCU：ESP32-C3
- Primary transport：Matter over Wi-Fi
- WF-RAC：本地 HTTP/HTTPS API（Transport 尚未實作）
- 控制原則：Local-first；日常控制不依賴雲端、MQTT 或 Internet

架構方向為 `WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi`。Matter Adapter 只使用高階 Device Model request，不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。

## 範圍與證據

本階段只記錄研究、架構、硬體策略、安全與 roadmap。Reference facts 依 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING` 分級。WF-RAC 使用者硬體的 protocol/runtime validation 目前為 `HARDWARE_TEST_PENDING`。

## 開發方式與專案規模

本專案採用 **AI-assisted development / vibe coding** 方式進行開發，主要透過 **ChatGPT** 與 **OpenAI Codex** 協作，形成 human-in-the-loop 的 AI-assisted engineering workflow。

- **開發者**：負責需求、硬體選擇、系統方向、實機量測與最終驗證。
- **ChatGPT**：協助架構、規格、技術討論、research、review 與開發任務拆解。
- **OpenAI Codex**：協助程式實作、文件／規格維護、測試／靜態驗證與 repository 維護。

AI 產生的程式、研究整理與設計建議不會直接視為完成品；涉及 WF-RAC 本地協議、ESP32、空調控制、網路通訊與 Matter 整合的內容，仍需經過實機測試、通訊行為驗證及人工判斷後，才納入正式設計。本段不代表 OpenAI 贊助、認證或參與本專案的硬體安全或空調控制決策。

### 專案規模

依目前 Git tracked files 的 physical lines 統計如下：

| 分類 | Physical lines |
| --- | ---: |
| Firmware（Phase 1A protocol core／Phase 1B device model／Phase 1C transport/API abstraction） | 1930 |
| 開發／驗證工具（tools、tests、GitHub Actions） | 2683 |
| Machine-readable protocol/spec contracts（specs） | 404 |
| 技術文件（README、AGENTS、docs） | 595 |
| 其他必要 tracked project files（.gitignore、.gitattributes） | 32 |
| **合計** | **5644** |

以上為 Git tracked files 的實體行數（physical lines），包含空白與註解；不包含第三方函式庫、downloaded dependencies、build／cache 與 generated artifacts。Firmware 指 portable C/C++ protocol component，不代表已完成 network runtime 或 hardware validation。

## License

TBD。正式公開前再決定 project license；本研究基準不新增 LICENSE。

詳見 [docs/roadmap.md](docs/roadmap.md)。
