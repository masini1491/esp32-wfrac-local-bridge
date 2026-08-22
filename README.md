# esp32-wfrac-local-bridge

這是將 Mitsubishi Heavy Industries WF-RAC 空調銜接至 Matter 的本地研究與架構基準。

研究 reference set 已補強，並記錄 AirconStat 與 MHI CNS/SPI-derived semantics 的高階關係；本專案仍處於 Phase 0，韌體與實機驗證目前暫緩。

Phase 0 的 machine-readable software foundation 已建立於 [`specs/wf-rac-contracts.json`](specs/wf-rac-contracts.json)，僅整理研究 contract，不包含 runtime implementation。

目前 software-only validation 流程為：`specs/wf-rac-contracts.json` → `tools/validate_contracts.py` → `tests/host/test_validate_contracts.py` → GitHub Actions。dependency-free host validator 與 37 個 targeted host tests 已通過，GitHub Actions software validation gate 亦已通過；`CONFIRMED_LOCAL = 0`，hardware/runtime validation 仍為 `HARDWARE_TEST_PENDING`。

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
| Firmware（Phase 0 尚未開始 firmware） | 0 |
| 開發／驗證工具（tools、tests、GitHub Actions） | 393 |
| Machine-readable protocol/spec contracts（specs） | 325 |
| 技術文件（README、AGENTS、docs） | 337 |
| 其他必要 tracked project files（.gitignore） | 14 |
| **合計** | **1069** |

以上為 Git tracked files 的實體行數（physical lines），包含空白與註解；不包含第三方函式庫、downloaded dependencies、build／cache 與 generated artifacts。Firmware 為 0 行，因本專案目前仍處於 Phase 0。

## License

TBD。正式公開前再決定 project license；本研究基準不新增 LICENSE。

詳見 [docs/roadmap.md](docs/roadmap.md)。
