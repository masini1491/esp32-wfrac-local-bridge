# Development discipline

本 repository 目前進入 Phase 1C — WF-RAC Transport Abstraction。允許既有 Phase 1A protocol core 與 Phase 1B Device Model 維護、portable transport contracts、request／response DTO、portable response envelope／getDeviceInfo bootstrap parsing、getAirconStat JSON／Base64／AirconStat integrity parsing、validated Protocol frame → Device Model observation／reconciliation mapping、ESP-IDF system-time adapter、explicit gettimeofday read／settimeofday seed／quality adoption boundary、FakeTransport／host-side unit tests、endpoint／scheme value types、bounded timeout value representation、portable API／transport orchestration、ESP-IDF-specific transport adapter、HTTP/TLS error classifier、bounded synchronous single-attempt HTTP POST／response collection、diagnostic ESP-IDF Wi-Fi source（RAM-only、one-shot、read-only getDeviceInfo wiring）與 ESP32-C6／ESP32-C3 compile/link integration。仍禁止實際 HTTP/HTTPS request execution、TLS handshake、SNTP、Wi-Fi runtime、mDNS runtime、NVS、real WF-RAC requests、scheme retry／polling runtime、hardware/network test、Matter Adapter／Matter runtime、MQTT、WebUI、credential handling，以及未經要求的 scope expansion。

上述 phase／capability 描述只界定 project technical boundary，不構成 execution authority；任何實作、runtime、hardware 或 coordination mutation 仍須有使用者當次明確授權。

正式 framework 是 ESP-IDF；ESP32-C6 為 primary、ESP32-C3 為 compatibility target；Matter-over-Wi-Fi 是 primary transport。固定 layering：WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi。

Matter Adapter 不得直接建 request、parse raw JSON、管理 Operator ID、retry、discovery 或 socket；Device Model 應與傳輸層及上層整合介面解耦，保持清楚的模組邊界與可測試性。未來 runtime 應採 non-blocking、bounded timeout/retry、bounded command queue、command serialization、polling/command coordination、state consistency 與 explicit availability。

使用 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING`，並記 repository、revision、license 與研究用途。不可未審 license copy、line-by-line translation、mechanical port 或 rename-and-claim。`mhi-ac-ctrl-esp32` 不是 WF-RAC HTTP protocol authority。

日常控制不得依賴雲端、MQTT、Internet、DNS 或 NTP。不得提交或輸出 Wi-Fi credentials、Operator ID、fabric/commissioning secrets、OTA signing key 或 WebUI secrets。加密 NVS、Secure Boot、Flash Encryption 目前只列 roadmap。

## Reference reading discipline

Reference reading must be topic-scoped。Future task 先讀 `docs/references/README.md`，再只讀直接相關的 `topics/*.md`；只有 provenance、source conflict 或 evidence audit 才讀 `sources/*.md`。不得預設完整讀 `docs/protocol-research.md` 或全部 reference files；evidence 不足時才逐層擴大 context。

## Common playbook routing

本專案採用 `masini1491/ai-development-playbook` 的 floating `main` 作為 common development baseline，AI mode 為 `ChatGPT+Codex`。本 `AGENTS.md` 僅保留 project mapping、project-native authority 與較嚴格例外；repository architecture、security、protocol、hardware 及其他 formal technical contracts 優先於 common playbook。依目前 task 由 `CHAT_INIT.md` 進入最低必要 owner，不得完整掃描 playbook，也不得把 routing 視為 execution、filesystem、network、credential 或下一 Stage 的授權。

Project-native technical owners 包括 `docs/architecture.md`、`docs/security.md`、`docs/roadmap.md`、`docs/references/README.md`／直接相關 topics、`specs/wf-rac-contracts.json` 及 current source/tests/evidence；普通 technical query 應在充分 evidence 後停止。ChatGPT Coordination Write Allowlist 僅包含 `/TASKS.md`、`/BACKLOG.md` 與 `/evidence/inbox/*.md`；這些 coordination/staging surface 不取代上述正式 authority。

`/evidence/inbox/*.md` 只保存 repo-safe、sanitized observation／provenance staging，預設不載入、不具 execution authority，也不是 architecture、protocol、security 或 validation canonical truth。不得先提交 raw secret/private evidence 再清理；既有 canonical `docs/references/sources/*` 不複製回 inbox，reconciliation 後應收斂為 canonical pointer／必要 delta 或移除。未列入 allowlist 的其他 path 維持 read-only。

- New-session bootstrap / task routing → `CHAT_INIT.md`; AI context / Hot-Cold-Evidence retrieval → `AI_CONTEXT.md`
- Git / permission / external-service → `REPOSITORY_EXECUTION.md`; debug / retry / validation → `DEBUG_VALIDATION.md`
- Architecture / upstream authority → `RESEARCH_ARCHITECTURE.md`; ESP32 / embedded / hardware → `EMBEDDED_PROJECTS.md`
- ChatGPT workflow / task admission / coordination → `CHATGPT_WORKFLOW.md`; Codex execution / reporting → `CODEX_EXECUTION.md`
- Windows / PowerShell / local toolchain（需要時） → `TOOLCHAIN.md`

External playbook reference 只表示應讀哪個 authority，不表示 execution environment 自動具有 network、filesystem 或 credential capability；需要額外 access 時仍依 routed permission/execution governance 處理。

## Project execution exceptions

本專案每次仍要求 `main`、clean working tree、無 local ahead/diverged，且只可 fast-forward-only 同步 `origin/main`；只有使用者當次明確授權的 task/stage 可執行。其餘 identity、permission、Git、queue、debug、retry、validation 與 toolchain mechanics 依上述 routed owners。

## Credential and mutation authorization boundary

WFRAC、Wi-Fi、Matter credential 不得成為 scope expansion來源；真實 password、token、Authorization header、private key、shared secret、Wi-Fi credential、Operator ID 或其他 credential material 不得出現在 Git、log、error report 或 final summary。WF-RAC provisioning／firmware 等敏感 mutation 仍遵守本 repository `docs/security.md` 的獨立安全與明確授權邊界；通用 permission、authorization 與 credential capability 方法論依 routing 取得。

## Windows / PowerShell toolchain contract

本 repository 的 Windows interactive/local development 正式 shell baseline 為 PowerShell 7／`pwsh`／`PSEdition Core`；不因此要求 GitHub Actions 或其他 cross-platform validation 改用 Windows。其餘 runtime/preflight/failure semantics 依 `TOOLCHAIN.md` routing 處理。

## Portable library extraction invariant

Repository 現階段維持 monorepo，不因未來 library 化而提前拆 repo、做 package distribution 或 semantic versioning。`wfrac_protocol`、`wfrac_device_model`、`wfrac_api`、`wfrac_transport`、`wfrac_client` 必須持續保持可抽離；platform/runtime concern 留在 adapter、runtime 或 firmware boundary，dependency direction 維持 `platform/runtime → portable core`。

Portable core 不得依賴 ESP-IDF、FreeRTOS、Wi-Fi、NVS、Matter 或 firmware runtime；portable public headers/API 不得洩漏 `esp_err_t`、ESP-IDF handles、FreeRTOS types、Wi-Fi/NVS/Matter types 或其他 platform-specific API。`wfrac_transport_espidf`、`wfrac_time_espidf` 及未來 Wi-Fi/NVS/Matter/firmware runtime 均屬 platform-specific，不屬 portable core。每次跨 component 修改都要確認 library readiness 為 `PRESERVED` 或 `ISSUE`。
