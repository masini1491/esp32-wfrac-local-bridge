# Development discipline

本 repository 目前進入 Phase 1C — WF-RAC Transport Abstraction。允許既有 Phase 1A protocol core 與 Phase 1B Device Model 維護、portable transport contracts、request／response DTO、portable response envelope／getDeviceInfo bootstrap parsing、getAirconStat JSON／Base64／AirconStat integrity parsing、validated Protocol frame → Device Model observation／reconciliation mapping、ESP-IDF system-time adapter、explicit gettimeofday read／settimeofday seed／quality adoption boundary、FakeTransport／host-side unit tests、endpoint／scheme value types、bounded timeout value representation、portable API／transport orchestration、ESP-IDF-specific transport adapter、HTTP/TLS error classifier、bounded synchronous single-attempt HTTP POST／response collection、diagnostic ESP-IDF Wi-Fi source（RAM-only、one-shot、read-only getDeviceInfo wiring）與 ESP32-C6／ESP32-C3 compile/link integration。仍禁止實際 HTTP/HTTPS request execution、TLS handshake、SNTP、Wi-Fi runtime、mDNS runtime、NVS、real WF-RAC requests、scheme retry／polling runtime、hardware/network test、Matter Adapter／Matter runtime、MQTT、WebUI、credential handling，以及未經要求的 scope expansion。

上述 phase／capability 描述只界定 project technical boundary，不構成 execution authority；任何實作、runtime、hardware 或 coordination mutation 仍須有使用者當次明確授權。

Software-only Matter exception：使用者明確授權的 Room Air Conditioner mapping／adapter contract 可實作與 host／compile 驗證，依賴方向僅為 adapter → Device Model，不引入 Matter SDK types 至 portable core。上述 Matter Adapter 禁止不適用於此 software-only contract；actual Matter stack/runtime、node/endpoint execution、commissioning、fabric、network/hardware 與未授權相鄰 scope 仍禁止。

Persistence compile-only exception：使用者明確授權的 typed portable persistence contract／ESP-IDF NVS adapter source 可做 fake host tests 與 C6/C3 compile/link；不受上述 generic NVS source 禁止限制。actual NVS runtime／hardware、production credential persistence、registration mutation、automatic time adoption 與未授權相鄰 scope 仍禁止；compile-ready 不代表 encrypted storage 或 secret-at-rest approval。

Diagnostic-only exception：`firmware/wfrac_bootstrap_probe` 可實作 default-off temporary SoftAP WebUI／APSTA、RAM-bound credentials 與每次 explicit submit 最多一次 bounded STA connect／read-only getDeviceInfo，僅限 implementation、offline host tests 與 compile/link validation。Production Provisioning/WebUI、persistent credential store、一般 Wi-Fi/network runtime 仍禁止；實際 Wi-Fi、HTTP/TLS、WF-RAC、flash／hardware execution 仍須獨立明確授權，不由此 exception 開啟。

正式 framework 是 ESP-IDF；ESP32-C6 為 primary、ESP32-C3 為 compatibility target；Matter-over-Wi-Fi 是 primary transport。固定 layering：WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi。

Matter Adapter 不得直接建 request、parse raw JSON、管理 Operator ID、retry、discovery 或 socket；Device Model 應與傳輸層及上層整合介面解耦，保持清楚的模組邊界與可測試性。未來 runtime 應採 non-blocking、bounded timeout/retry、bounded command queue、command serialization、polling/command coordination、state consistency 與 explicit availability。

使用 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING`，並記 repository、revision、license 與研究用途。不可未審 license copy、line-by-line translation、mechanical port 或 rename-and-claim。`mhi-ac-ctrl-esp32` 不是 WF-RAC HTTP protocol authority。

日常控制不得依賴雲端、MQTT、Internet、DNS 或 NTP。不得提交或輸出 Wi-Fi credentials、Operator ID、fabric/commissioning secrets、OTA signing key 或 WebUI secrets。加密 NVS、Secure Boot、Flash Encryption 目前只列 roadmap。

## Reference reading discipline

Reference reading must be topic-scoped。Future task 先讀 `docs/references/README.md`，再只讀直接相關的 `topics/*.md`；只有 provenance、source conflict 或 evidence audit 才讀 `sources/*.md`。不得預設完整讀 `docs/protocol-research.md` 或全部 reference files；evidence 不足時才逐層擴大 context。

## Common playbook routing

- 普通 bounded WF-RAC technical work先走 project-native technical owners；evidence 足夠即停止，不因採用 shared Playbook 而啟用其他 shared governance。
- 只有 current task/action 確實需要 shared governance 時，才依下方結構化 adoption declaration 的 baseline 啟用 shared governance；所需入口與 exact-revision 路由見該 declaration。

`/evidence/inbox/*.md` 只保存 repo-safe、sanitized observation／provenance staging，預設不載入、不具 execution authority，也不是 architecture、protocol、security 或 validation canonical truth。不得先提交 raw secret/private evidence 再清理；既有 canonical `docs/references/sources/*` 不複製回 inbox，reconciliation 後應收斂為 canonical pointer／必要 delta 或移除。未列入 allowlist 的其他 path 維持 read-only。

## AI Development Playbook baseline

- Playbook repository: `masini1491/ai-development-playbook`
- Playbook baseline: `main`
- Project AI mode: `ChatGPT+Codex`
- The declared baseline's `REPORTING.md` applies to substantive user-facing engineering replies even when other shared Playbook governance is not activated; this thin reporting pointer does not activate `CHAT_INIT.md` or expand task, write, execution, permission, credential or validation authority.
- When shared governance is required, resolve the declared baseline to an exact revision and enter that revision's `CHAT_INIT.md`; follow its minimum routing for the current task.

## Authority boundary

本 repository 的 project-specific authority 與 technical source of truth（包括 architecture、security、protocol、hardware、current task/evidence）優先於 shared Playbook。採用 Playbook 本身不會新增 task、write、execution、deployment、permission、credential 或 validation authority；shared routing 也不會跳過本 repository 的 user authorization、scope、security 或 hardware restrictions。

## Project-specific minimum contract

- Canonical technical source(s): `docs/architecture.md`, `docs/security.md`, `docs/roadmap.md`, `docs/references/README.md`, `specs/wf-rac-contracts.json`, current source/tests/evidence
- Current coordination surface: `TASKS.md`
- ChatGPT coordination write allowlist: `/TASKS.md`, `/BACKLOG.md`, `/evidence/inbox/*.md`
- Required validation: task-scoped targeted validation, canonical read-back, and `git diff --check` as applicable
- Project-specific exceptions or restrictions: Phase 1C no-real-network/hardware boundary, credential secrecy, ESP-IDF/C6/C3 targets, portable-library boundary, and user-authorized scope only

## Project execution exceptions

本專案每次仍要求 `main`、clean working tree、無 local ahead/diverged，且只可 fast-forward-only 同步 `origin/main`；只有使用者當次明確授權的 task/stage 可執行。共通 identity、permission、Git、queue、debug、retry、validation 與 toolchain mechanics 依 shared Playbook routing 在需要時載入。

## Credential and mutation authorization boundary

WFRAC、Wi-Fi、Matter credential 不得成為 scope expansion來源；真實 password、token、Authorization header、private key、shared secret、Wi-Fi credential、Operator ID 或其他 credential material 不得出現在 Git、log、error report 或 final summary。WF-RAC provisioning／firmware 等敏感 mutation 仍遵守本 repository `docs/security.md` 的獨立安全與明確授權邊界；通用 permission、authorization 與 credential capability 方法論依 routing 取得。

## Windows / PowerShell toolchain contract

本 repository 的 Windows interactive/local development 正式 shell baseline 為 PowerShell 7／`pwsh`／`PSEdition Core`；不因此要求 GitHub Actions 或其他 cross-platform validation 改用 Windows。

## Portable library extraction invariant

Repository 現階段維持 monorepo，不因未來 library 化而提前拆 repo、做 package distribution 或 semantic versioning。`wfrac_protocol`、`wfrac_device_model`、`wfrac_api`、`wfrac_transport`、`wfrac_client`、`wfrac_persistence` 必須持續保持可抽離；platform/runtime concern 留在 adapter、runtime 或 firmware boundary，dependency direction 維持 `platform/runtime → portable core`。

Portable core 不得依賴 ESP-IDF、FreeRTOS、Wi-Fi、NVS、Matter 或 firmware runtime；portable public headers/API 不得洩漏 `esp_err_t`、ESP-IDF handles、FreeRTOS types、Wi-Fi/NVS/Matter types 或其他 platform-specific API。`wfrac_transport_espidf`、`wfrac_time_espidf` 及未來 Wi-Fi/NVS/Matter/firmware runtime 均屬 platform-specific，不屬 portable core。每次跨 component 修改都要確認 library readiness 為 `PRESERVED` 或 `ISSUE`。
