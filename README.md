# esp32-wfrac-local-bridge

這是將 Mitsubishi Heavy Industries WF-RAC 空調銜接至 Matter 的本地研究與架構基準。

研究 reference set 已補強，並記錄 AirconStat 與 MHI CNS/SPI-derived semantics 的高階關係；本專案已進入 Phase 1C WF-RAC Transport Abstraction，實機驗證目前暫緩。

Phase 0 的 machine-readable software foundation 已建立於 [`specs/wf-rac-contracts.json`](specs/wf-rac-contracts.json)，僅整理研究 contract，不包含 runtime implementation。

目前 software-only validation 流程為：`specs/wf-rac-contracts.json` → `tools/validate_contracts.py` → `tests/host/test_validate_contracts.py`，並新增 `components/wfrac_protocol/` 的 C++17 CRC16／AirconStat structural framing、basic fixed-state decoder、四個 safe command intents、capability profile decoder、operation-data raw segment parser/classifier、Aircon block serializer、safe query builder、standard Base64 codec 與 generic full AirconStat composer host tests，皆由 GitHub Actions 執行。Python host tests 目前為 66 個；C++ protocol core tests 為 1 個 executable；`CONFIRMED_LOCAL = 0`，hardware/runtime validation 仍為 `HARDWARE_TEST_PENDING`。

最小 host validation commands：

```text
python -m py_compile tools/validate_contracts.py tests/host/test_validate_contracts.py
python tools/validate_contracts.py
python -m unittest tests.host.test_validate_contracts
```

本地開發驗證需要 Python 3、支援 C++17 的 MSYS2 UCRT64 GCC/G++ 與 ESP-IDF v6.0.2；ESP32-C6 為 primary target，ESP32-C3 為 compatibility target。Windows interactive/local development 使用 PowerShell 7（`pwsh`、`PSEdition Core`）；Windows PowerShell 5.1（`powershell.exe`）不屬正式 PowerShell validation runtime。這不取代既有 Python/C++/ESP-IDF tooling，也不要求 GitHub Actions 改用 Windows。

獨立的 [ESP-IDF compile/link workflow](.github/workflows/esp-idf-compile.yml) 使用官方 `espressif/idf:v6.0.2` image，在相關 `main` push／PR 持續編譯 `idf_compile_smoke` 與 default-inert `wfrac_bootstrap_probe`，各涵蓋 ESP32-C6／ESP32-C3；亦可手動觸發。此四格 gate 只提供遠端 compile/link evidence，不執行 firmware、flash、Wi-Fi 或 WF-RAC request；既有 Python／host C++ CI 保持獨立。初次四格 [remote validation run](https://github.com/masini1491/esp32-wfrac-local-bridge/actions/runs/37220534600) 已 PASS。

## 專案狀態

目前是 **Phase 1C — WF-RAC Transport Abstraction**。Phase 1A Protocol Core #1–#6 與 Phase 1B Device Model foundations 已完成；目前另有 portable request executor、typed API request builders、response envelope／getDeviceInfo bootstrap parser，以及 getAirconStat contents → Base64 → AirconStat structural／dual-CRC integrity gate → RECEIVE semantic decode → DeviceState observation。Device Model boundary會再次檢查雙 CRC；availability 仍是 runtime-owned，observation convergence 不等於 transport confirmation。ESP-IDF `EspSystemTimeProvider` 以 explicit seed/adopt 將 POSIX system clock 提供給既有 64-bit TimeProvider path，初始 unavailable，沒有自動同步或 trust inference。response `airconId` 為 optional，unknown contents fields 可 bounded-skip，result semantics 保持 command-specific。ESP-IDF adapter 仍只提供 single-attempt HTTP POST source integration。`firmware/wfrac_bootstrap_probe` 已提供 default-inert、RAM-only、one-shot read-only `getDeviceInfo` diagnostic wiring，狀態為 **COMPILE-READY**，不是 hardware/network validated。actual bootstrap probe、mDNS、persistent identity、certificate trust acquisition 與 platform runtime 仍暫緩；bounded software-only Matter mapping／adapter contract 已建立，完整 Matter mapping、actual runtime／commissioning 仍暫緩。

- Framework：ESP-IDF
- Primary MCU：ESP32-C6
- Compatibility MCU：ESP32-C3
- Primary transport：Matter over Wi-Fi
- WF-RAC：本地 HTTP/HTTPS API（ESP-IDF single-attempt transport adapter 已實作；runtime 未實作）
- 控制原則：Local-first；日常控制不依賴雲端、MQTT 或 Internet

`wfrac_client::RuntimeOrchestrator` 已提供 portable、caller-driven synchronous step core：bounded/coalescing command queue、poll/command serialization、read-only retry、explicit availability 與既有 post-write verifier整合。Monotonic clock 與 Unix TimeProvider 分離，所有 poll/settle/retry/deadline/baseline-age timing 由 caller policy 提供；不重送 write，不建立 production scheduler/thread，也沒有 hardware/network execution evidence。

架構方向為 `WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi`。Matter Adapter 只使用高階 Device Model request，不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。

`wfrac_matter_adapter` 提供 SDK-free、host-testable 的 [RAC mapping contract](docs/references/topics/matter-rac-mapping.md)：available observed power／Off/Cool/Heat／對應 setpoint與 nullable controller-temperature proxy；unknown／unsupported 不造預設，desired／pending 不做 optimistic projection。Write 只映射單一 Device Model intent，既有 runtime／protocol 仍負責 admission、dispatch 與 verification；Fan／Auto／Dry 等 unresolved mapping 不開啟，也未建立 Matter node／endpoint／commissioning。

本專案的方向是 portable WF-RAC C++ library stack、ESP-IDF reference implementation 與 future Matter bridge firmware；目前維持 monorepo，library packaging、versioning 與外部 package distribution 尚未開始。

`wfrac_persistence` 與 `wfrac_persistence_espidf` 已建立 [typed persistence boundary](docs/references/topics/persistence-boundary.md)：bounded UTF-8 Operator ID、per-device confirmed scheme、64-bit historical checkpoint，typed load/store/erase與schema/corruption分類。NVS adapter僅source／compile-link，不自動initialize／recover partition或採用loaded values；checkpoint無trusted quality。Fake tests不是flash durability evidence，production secret-at-rest approval／encrypted storage／actual NVS與consumer integration仍pending。

## 範圍與證據

Portable operation-data已提供 canonical `0x80` indoor/outdoor network lookup（exact deci-Celsius）、`0x94/OP1=0x10` current-run energy（quarter-kWh／exact Wh），以及 `0x13` EEV-related network OP2 raw pulse/value byte decoder；皆保留raw bytes。`0x13` 不代表百分比、校準閥門開度、full-open count或direct CNS counter等價；[decoder boundary／provenance](docs/references/topics/operation-data.md) 限於software host／compile驗證，不升級其他diagnostic formulas、Device Model/Matter整合或local hardware evidence。

另提供明確 `INFERRED` 的outdoor frequency（deci-Hz）、current（exact rational A）及bounded discharge（half-degree candidate）read-side conversions；raw保留，低端discharge no-value／無clamp，不代表calibration或local physical truth，未接入Device Model／Matter。

Diagnostic firmware 另提供 default-off `WFRAC_PROBE_WEBUI_ENABLED`：embedded WebUI／temporary WPA2 SoftAP，只由 explicit bounded POST 觸發一次 APSTA connect 與 read-only getDeviceInfo，共用原 one-shot executor。需本地 unique AP passphrase，預設空值 fail closed；STA／WF-RAC inputs 不寫 NVS、不回顯／log，HTTPS 仍需 explicit PEM trust，不自動降級。詳細安全邊界見 [security](docs/security.md)。狀態為 **COMPILE-READY**，不是 production provisioning／hardware validated。[Remote six-cell CI](https://github.com/masini1491/esp32-wfrac-local-bridge/actions/runs/37227175448) 保留原四格 default gates，新增 WebUI-enabled C6／C3 full-link，均 PASS；[host CI](https://github.com/masini1491/esp32-wfrac-local-bridge/actions/runs/37227175431) 含 offline form regression 亦 PASS，tested revision `4b937eaf1a21fc86f3a9575be1431f750ad46554`。`webui.defaults` 是 synthetic compile-only fixture，不得 flash；實機必須另行明確授權並提供本地 unique configuration。本次 firmware network／hardware executions = 0。

本階段只記錄研究、架構、硬體策略、安全與 roadmap。Reference facts 依 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING` 分級。WF-RAC 使用者硬體的 protocol/runtime validation 目前為 `HARDWARE_TEST_PENDING`。

## 開發方式與專案規模

本專案採用 **AI-assisted development / vibe coding** 方式進行開發，主要透過 **ChatGPT** 與 **OpenAI Codex** 協作，形成 human-in-the-loop 的 AI-assisted engineering workflow。

- **開發者**：負責需求、硬體選擇、系統方向、實機量測與最終驗證。
- **ChatGPT**：協助架構、規格、技術討論、research、review 與開發任務拆解。
- **OpenAI Codex**：協助程式實作、文件／規格維護、測試／靜態驗證與 repository 維護。

AI 產生的程式、研究整理與設計建議不會直接視為完成品；涉及 WF-RAC 本地協議、ESP32、空調控制、網路通訊與 Matter 整合的內容，仍需經過實機測試、通訊行為驗證及人工判斷後，才納入正式設計。本段不代表 OpenAI 贊助、認證或參與本專案的硬體安全或空調控制決策。

### 專案規模

依目前 repository files 的 physical lines 統計如下：

| 分類 | Physical lines |
| --- | ---: |
| Firmware（components、diagnostic bootstrap firmware） | 5623 |
| 開發／驗證工具（tools、tests、GitHub Actions、ESP-IDF compile harness） | 5303 |
| Machine-readable protocol/spec contracts（specs） | 406 |
| 技術文件（README、AGENTS、docs） | 937 |
| 其他必要 tracked project files（.gitignore、.gitattributes） | 33 |
| **合計** | **12302** |

以上依 tracked Git content 統計實體行數（physical lines），包含空白與註解；不包含 TASKS／BACKLOG coordination、evidence staging、第三方函式庫、downloaded dependencies、build／cache 與 generated artifacts。Firmware 包含 portable components、ESP-IDF adapters 與 diagnostic source，不代表已完成 network runtime 或 hardware validation。

## License

TBD。正式公開前再決定 project license；本研究基準不新增 LICENSE。

詳見 [docs/roadmap.md](docs/roadmap.md)。
