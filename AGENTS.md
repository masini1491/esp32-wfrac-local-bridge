# Development discipline

本 repository 目前進入 Phase 1C — WF-RAC Transport Abstraction。允許既有 Phase 1A protocol core 與 Phase 1B Device Model 維護、portable transport contracts、request／response DTO、portable response envelope／getDeviceInfo bootstrap parsing、getAirconStat JSON／Base64／AirconStat integrity parsing、validated Protocol frame → Device Model observation／reconciliation mapping、ESP-IDF system-time adapter、explicit gettimeofday read／settimeofday seed／quality adoption boundary、FakeTransport／host-side unit tests、endpoint／scheme value types、bounded timeout value representation、portable API／transport orchestration、ESP-IDF-specific transport adapter、HTTP/TLS error classifier、bounded synchronous single-attempt HTTP POST／response collection 與 ESP32-C6／ESP32-C3 compile/link integration。仍禁止實際 HTTP/HTTPS request execution、TLS handshake、SNTP、Wi-Fi runtime、mDNS runtime、NVS、real WF-RAC requests、scheme retry／polling runtime、hardware/network test、Matter Adapter／Matter runtime、MQTT、WebUI、credential handling，以及未經要求的 scope expansion。

正式 framework 是 ESP-IDF；ESP32-C6 為 primary、ESP32-C3 為 compatibility target；Matter-over-Wi-Fi 是 primary transport。固定 layering：WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi。

Matter Adapter 不得直接建 request、parse raw JSON、管理 Operator ID、retry、discovery 或 socket；Device Model 應與傳輸層及上層整合介面解耦，保持清楚的模組邊界與可測試性。未來 runtime 應採 non-blocking、bounded timeout/retry、bounded command queue、command serialization、polling/command coordination、state consistency 與 explicit availability。

使用 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING`，並記 repository、revision、license 與研究用途。不可未審 license copy、line-by-line translation、mechanical port 或 rename-and-claim。`mhi-ac-ctrl-esp32` 不是 WF-RAC HTTP protocol authority。

日常控制不得依賴雲端、MQTT、Internet、DNS 或 NTP。不得提交或輸出 Wi-Fi credentials、Operator ID、fabric/commissioning secrets、OTA signing key 或 WebUI secrets。加密 NVS、Secure Boot、Flash Encryption 目前只列 roadmap。

失敗分類：`SOURCE`、`TOOLCHAIN`、`ENVIRONMENT`、`INFRASTRUCTURE`、`SERVICE`、`HARDWARE_REQUIRED`。同一 infrastructure/auth/toolchain/service root cause 最多自動 retry 1 次；第二次停止，不重掃、不重複 download、不因 toolchain 問題修改 production source。沒有明確要求不得自動實作；保留 hardware truth，避免宣稱未驗證能力。
## Reference reading discipline

Reference reading must be topic-scoped。Future task 先讀 `docs/references/README.md`，再只讀直接相關的 `topics/*.md`；只有 provenance、source conflict 或 evidence audit 才讀 `sources/*.md`。不得預設完整讀 `docs/protocol-research.md` 或全部 reference files；evidence 不足時才逐層擴大 context。

## Remote sync and shared task queue

每次 task 開始時，local repository 可能落後 GitHub；在依賴 local `AGENTS.md` 或 `TASKS.md` 前，先確認 repository identity、`git status --short`、current branch、HEAD，以及 merge/rebase/cherry-pick 是否進行中，再執行 `git fetch origin`。只有 working tree clean、branch 正確、local `main` 沒有 ahead/diverged 且可由 `origin/main` fast-forward 時，才使用 fast-forward-only 同步 remote source of truth；dirty、unexpected branch、local ahead/diverged、無法 fast-forward 或未完成的 merge/rebase/cherry-pick 都必須 STOP。

不得使用 `reset`、force push、自行 merge/rebase、stash，或刪除／丟棄未知 user work。同步完成後才讀最新 `AGENTS.md`／`TASKS.md`。根目錄若存在 `TASKS.md`，它是 ChatGPT／Codex 共用的 temporary unfinished-work queue；Codex 每次只執行使用者當次明確授權的 task/stage，不得因 queue 存在而自行完成其他項目。成功完成並驗證授權 task 後，必須移除或更新對應 entry；不得保留 Completed/changelog，若已無 unfinished work 則刪除 `TASKS.md`。`TASKS.md` 不得凌駕 architecture、spec、security 或 protocol evidence 等正式 source of truth；同步後若 `TASKS.md` 不存在，不得依賴舊 local copy、舊 prompt 或記憶執行已移除的 task。

## Permission-Gated Operation

對目前已授權 task/stage 所必需的 Git、build、test、toolchain 或 file operation，若因 sandbox、filesystem 或 execution permission 被拒絕，先視為可由使用者授權解除的 permission gate，不立即判定 repository、toolchain 或 environment 故障。若執行環境支援 escalation，向使用者要求完成原必要操作所需的最小權限，並說明 command/operation、必要性與權限類型；批准後只重試原本被阻擋的操作，不擴大 scope。

單純 permission denial 不消耗 operational retry 額度。若使用者拒絕、環境無法要求權限，或取得權限後同一操作仍失敗，才依實際 evidence 分類 `ENVIRONMENT`／`INFRASTRUCTURE`／`TOOLCHAIN`／`SERVICE` 等 operational failure，再套用既有 retry／STOP 規則。`git fetch origin` 遇到 `.git/FETCH_HEAD: Permission denied`、Git lock/ref metadata filesystem denial 或 sandbox repository-metadata write denial 時，先依本規則要求安全 fetch 的最小權限；授權後原 fetch 仍失敗才分類並 STOP。

不得以 permission workaround 取代授權：禁止 `sudo`、`chmod -R 777`、`reset --hard`、force push、自行刪除 `.git/FETCH_HEAD`、未確認 root cause 就刪除 `.git/index.lock` 或其他 lock、重新 clone 覆蓋 working tree、stash/delete/discard unknown user work、自行 merge/rebase/cherry-pick，或使用另一個 repository 規避問題。Permission-gate handshake 不代表可開始其他 stage、修改未授權檔案或執行高風險 Git 操作。

## Portable library extraction invariant

Repository 現階段維持 monorepo，不因未來 library 化而提前拆 repo、做 package distribution 或 semantic versioning。`wfrac_protocol`、`wfrac_device_model`、`wfrac_api`、`wfrac_transport`、`wfrac_client` 必須持續保持可抽離；platform/runtime concern 留在 adapter、runtime 或 firmware boundary，dependency direction 維持 `platform/runtime → portable core`。

Portable core 不得依賴 ESP-IDF、FreeRTOS、Wi-Fi、NVS、Matter 或 firmware runtime；portable public headers/API 不得洩漏 `esp_err_t`、ESP-IDF handles、FreeRTOS types、Wi-Fi/NVS/Matter types 或其他 platform-specific API。`wfrac_transport_espidf`、`wfrac_time_espidf` 及未來 Wi-Fi/NVS/Matter/firmware runtime 均屬 platform-specific，不屬 portable core。每次跨 component 修改都要確認 library readiness 為 `PRESERVED` 或 `ISSUE`。
