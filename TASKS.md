# Unfinished work queue

此檔只追蹤尚未完成、Blocked、Deferred 或待驗證工作。完成項目應直接移除；Git history 才是完成紀錄。不得因看到本檔而自行執行未明確授權的工作。

## Codex 共通執行規則

- 每次只執行使用者當次明確授權的 stage；不得因本佇列存在就自行處理其他項目。
- **TASKS.md 使用判準：** `TASKS.md` 是「未完成、需要追蹤、需要未來再次執行」的 queue，不是所有微小修改都必須先建立 task。若只是一次性、已知位置、已知修改內容、風險低，而且完成後沒有後續追蹤價值的小型 maintenance，不必特地寫入 `TASKS.md`；可由 ChatGPT 直接產生一次性的短 Codex Prompt。典型例子包括：單行或少量文件 wording 修正、已知位置的 typo / 過期術語修正、單純排序/格式/標題/註解整理、不影響 project behavior / architecture / protocol / security / hardware / validation state 的小型 documentation maintenance，以及已知 root cause、scope 極小且完成後不需保留為 unfinished queue 的工作。這類 direct maintenance 預設建議 **Luna / Low / Context Level 0→1 / Focused patch / Agent 1**；Codex 仍必須遵守 `AGENTS.md` 的 remote-sync、scope、Git、validation、commit/push 規則。
- **應寫入 TASKS.md 的情況：** 需要後續追蹤；目前 Blocked / Deferred / Pending-validation；工作分成多個 Stage；有 dependency / trigger；root cause 尚未完全確認；可能需要之後接續 implementation；會實質影響 project behavior、architecture、protocol、security、hardware、persistence、runtime state 或重要 validation state；或若現在不記錄，之後容易遺漏。
- **Remote-sync bootstrap：** Codex 本機 repository 可能落後 GitHub。每次 task 開始，在依賴 local `AGENTS.md` / `TASKS.md` 內容前，先確認 repository identity、`git status --short`、branch、HEAD，然後 `git fetch origin`。若目前為預期 `main`、working tree clean，且 local `main` 可由 `origin/main` fast-forward，使用 fast-forward-only 方式同步（例如 `git pull --ff-only origin main`）；若已與 `origin/main` 相同則直接繼續。若 dirty、unexpected branch、local ahead/diverged、無法 fast-forward，或 merge/rebase/cherry-pick 未完成，STOP 並回報，不得覆蓋、reset、stash/丟棄未知變更或自行 merge/rebase。**只有完成這個同步判斷後，才以最新 local `AGENTS.md` / `TASKS.md` 為 task source。**
- 同步後讀 `AGENTS.md`、本 `TASKS.md` 與該 stage 直接相關檔案；依 Level 0→5 漸進式擴張，禁止預設 repo-wide 掃描。若同步後根目錄沒有 `TASKS.md`，不得憑舊記憶執行已移除的 stage；依使用者當次明確指令處理或 STOP 詢問。
- Git safety：不得 reset --hard、force push、丟棄或改寫未知變更；不得為了讓工作樹變乾淨而自行 stash/delete user work。
- Debug / patch 順序：Evidence → Root Cause → Focused Patch → Targeted Validation。Root cause 只能標為 `CONFIRMED ROOT CAUSE`、`HIGH-CONFIDENCE LIKELY ROOT CAUSE`、`INSUFFICIENT OBSERVABILITY`；只有前兩者可直接 patch，第三種先加最小觀測或 STOP。
- 同一 root cause 自動 retry 最多 1 次；第二次失敗時分類 `SOURCE` / `TOOLCHAIN` / `ENVIRONMENT` / `INFRASTRUCTURE` / `SERVICE` / `HARDWARE_REQUIRED`，不要無限重跑。
- Validation 由小到大：static/targeted verifier → targeted test → related build → 必要 matrix → full regression。不要為小 patch 預設跑最大驗證。
- A–D 預設 **Context：Default**、**Execution mode：Normal / single-agent**；不得使用 1M context、Fast、Ultra 或 Multi-Agent，除非該 stage 明確記載升級條件且有新 evidence 支持。
- 若 Luna 在已指定 root cause / patch direction 下仍無法安全完成，保留 evidence 再升 Terra；不得無證據直接升 Sol。
- 成功驗證後必須同步更新本檔：**刪除整個已完成 stage 的 prompt/metadata 與對應 inventory 項目**，不要留下 Completed 記錄。若尚有 Deferred/Blocked/TODO 則保留本檔；完全無未完成工作時刪除 `TASKS.md`。
- Commit/push 前 `git diff --check`；只提交本 stage scope。禁止 force push/history rewrite。
- Codex final report 保持精簡，只列：`Root cause / research gap`、`Modified files`、`Validation`、`Pending / TASKS.md state`；不要重述大量背景。

### Codex 短指令啟動格式

使用者可在 Codex UI 手動選好本 stage 指定的模型與推理強度後，只貼短指令：

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP）。同步後讀根目錄 `AGENTS.md` 與 `TASKS.md`，完整執行 `TASKS.md` 的「<Stage 名稱>」，只執行該 stage。成功驗證後依 `TASKS.md` 規則更新 queue、commit 並 push。

## Codex staged execution plan — 流量恢復後依序執行

### Stage A1 — Parser regression follow-up

**推薦模型：** Luna  
**推理強度：** Medium  
**推薦理由：** Stage A 後 code review 已定位兩個 bounded regression；root cause 明確，屬小型 parser correctness / memory-safety patch + targeted tests，不需要 Terra。  
**是否值得先用較便宜模型做前置蒐證：** 否；直接以目前 source 與新增 regression tests 修正。  
**Context 建議：** Default / Level 0→1；只讀 `components/wfrac_api/src/api_response.cpp` 與 `tests/host_cpp/test_api_response.cpp`，evidence 不足才擴張。  
**Execution mode 建議：** Normal / single-agent。  
**依賴：** 原 Stage A commit `17a46cdddcd9ea2519959a5b68e0ea88aeebd96c` 已完成；本 follow-up 必須先於 Stage B。  
**成功後：** 刪除本 Stage A1 整段與 inventory 中 `→ Stage A1` 項目，再 commit/push。

**Codex Prompt：**

> 執行「Stage A1 — Parser regression follow-up」。先依 `AGENTS.md` 做 remote-sync bootstrap，再只讀目前 `api_response.cpp` 與 API response host tests；不要執行 Stage B/C/D 或 Deferred。
>
> 已確認兩個 regression：
> 1. `decode()` 的 output-capacity 判斷目前使用類似 `n > out.size - w - 1U`。當 `out.size == 0` 且 `out.data != nullptr`，或 `w == out.size` 時，減法可發生 unsigned underflow，使 logical zero-capacity / exhausted-capacity view 在回傳 `output_too_small` 前仍可能寫入 caller storage。改成不會 underflow 的 remaining-capacity 判斷；在任何 write 前先證明至少有 `n` bytes payload + 1 byte NUL capacity。新增 sentinel regression：zero-capacity non-null storage 與 exact/exhausted capacity failure 時 caller bytes不得被修改超出 API contract。
> 2. `Reader::integer()` 目前在 `number()` 內部 `ws()` 之前先保存 `start` / `start_remaining`；因此合法 JSON value whitespace（例如 `{"command":"getDeviceInfo","result": 1}` 或 contents 的 `"apMode": 2`）會被算進 consumed span，造成 strict integer解析誤判。修正 cursor snapshot 順序或等價邏輯，使 JSON whitespace在 integer token 前合法，同時維持 strict integer規則：fraction/exponent仍對 known integer field回 `invalid_field_type` / 既有適當錯誤，不放寬 `+1`、leading zero等 malformed grammar。
>
> Targeted tests至少新增：known `result` 前有一個/多個 JSON whitespace、`apMode` 前有 whitespace；zero-capacity non-null storage sentinel unchanged；exact insufficient capacity sentinel unchanged；原 Stage A unknown-number grammar、embedded NUL、truncated Unicode、failure-atomic metadata tests不得退步。
>
> 只做 focused patch，不改 public API、不碰 UTF-8 semantic work（留給 Stage B）、Transport/Device Model/ESP-IDF runtime。先跑 API response targeted test，再跑既有 host regressions與 `git diff --check`；不需要 ESP-IDF build。
>
> 全部 PASS 後從 `TASKS.md` 刪除整個 Stage A1 與所有 `→ Stage A1` inventory 項目；Stage B保留並成為下一個可執行 stage。commit/push `main`。

### Stage B — JSON string semantics / UTF-8 hardening

**推薦模型：** Luna  
**推理強度：** Medium  
**推薦理由：** 行為規格可完全預先定義，主要是 bounded JSON-string comparator / UTF-8 validation 與測試；先用 Luna 可控制成本。  
**是否值得先用較便宜模型做前置蒐證：** 否；Luna 已足夠。  
**Context 建議：** Default；以 Stage A/A1 後的 `api_response.cpp`、public header、response tests 為中心。  
**Execution mode 建議：** Normal / single-agent；不使用 Fast / Ultra / Multi-Agent。  
**依賴：** Stage A1 PASS 且 Stage A1 已從 `TASKS.md` 移除。  
**升級條件：** 若正確 raw UTF-8 validation / decoded-key comparison 必須大規模改寫 parser 或改 public API，STOP 並帶 evidence 升 Terra Medium。  
**成功後：** 刪除本 Stage B 整段與 inventory 中 Stage B 項目，再 commit/push。

**Codex Prompt：**

> 執行「Stage B — JSON string semantics / UTF-8 hardening」。必須以 Stage A/A1 已完成的 `main` 為 baseline。遵守 `AGENTS.md`、`TASKS.md` 共通規則；先完成 remote-sync bootstrap，再只讀目前 `api_response.cpp`、public response header 與直接相關 host tests。
>
> 僅處理：
> 1. known JSON keys 與 top-level `command` 必須依 **decoded JSON string value** 比較，使合法 escape（例如 `\uXXXX`）與等價 raw spelling語意相同。語意相同但 spelling 不同的 duplicate known key（例如 `airconId` + `aircon\u0049d`）仍必須被視為 duplicate/reject；不要放寬 duplicate-key 或 known-field type rules。
> 2. raw non-ASCII UTF-8 bytes不得逐 byte 當 Unicode code point 再編碼；合法 raw UTF-8必須 byte-preserving，`\uXXXX` / surrogate pair 則正確輸出 UTF-8。
> 3. JSON parser 的 string validation 必須適用於 keys、known values、unknown/skipped string values，不得因 value 被 skip 就繞過 UTF-8 validation。
> 4. 驗證並拒絕 malformed UTF-8：bad continuation、truncated sequence、overlong encoding、UTF-8 encoded surrogate、code point > U+10FFFF。Stage A 的 embedded U+0000 identity rejection 必須保持。
> 5. targeted tests 至少包含 raw `é`、中文字串 byte-identical、`\u00E9` 等價 decode、escaped command/key、semantic duplicate key，以及上述 malformed UTF-8 cases。
>
> 優先採 bounded/no-heap helper 或 streaming comparison；不得引入 heap/external JSON library，不得重寫整個 parser，不得碰 Transport/Device Model/ESP-IDF runtime。targeted API response tests PASS 後跑既有 host regressions。
>
> 若必須改 public API 或大規模 parser rewrite，STOP，保留 evidence 並建議 Terra Medium；不要自己升級 scope。
>
> 成功後從 `TASKS.md` 刪除整個 Stage B 區塊與所有 `→ Stage B` inventory 項目，`git diff --check` 後 commit/push。

### Stage C — ESP-IDF transport diagnostics correctness

**推薦模型：** Luna  
**推理強度：** Medium  
**推薦理由：** 根因與 ESP-IDF v6.0.2 contract 已定位，工作是 adapter-specific taxonomy/contract patch + compile validation；不需要 Terra 做架構決策。  
**是否值得先用較便宜模型做前置蒐證：** 否；不要重新搜尋網路或整個 repo。若 repo 內 pinned ESP-IDF evidence 與 source 衝突才停止並升級蒐證。  
**Context 建議：** Default；只讀 `wfrac_transport_espidf` classifier/transport、直接測試/compile smoke，以及必要的已 pinned transport/TLS reference topic。  
**Execution mode 建議：** Normal / single-agent；不使用 Fast / Ultra / Multi-Agent。  
**依賴：** Stage A1 PASS；為保持線性 history，預設在 Stage B 後執行。  
**升級條件：** 若 ESP-IDF v6.0.2 public error constants / cert contract 與 pinned evidence 不符，或無法以現有 classifier evidence type 正確區分，不猜；STOP 並升 Terra Medium 做 focused evidence review。  
**成功後：** 刪除本 Stage C 整段與 inventory 中 Stage C 項目，再 commit/push。

**Codex Prompt：**

> 執行「Stage C — ESP-IDF transport diagnostics correctness」。遵守 `AGENTS.md`、`TASKS.md` 共通規則。先完成 remote-sync bootstrap，再只讀 `wfrac_transport_espidf` error classifier、`EspHttpTransport`、直接相關 tests/compile-smoke，以及必要的 pinned ESP-IDF transport/TLS reference topic；不要重新研究整個 repo或做泛化網搜。
>
> 已知問題與要求：
> 1. classifier 必須先尊重 certificate verify evidence：`tls_verify_flags != 0` → `tls_identity_failure`。
> 2. ESP-TLS / socket / perform evidence中可明確辨識的 connection timeout → `timeout`；hostname resolution、socket creation、failed TCP connect / network reachability → `connection_failure`；local invalid argument/invalid transport/config misuse → `invalid_argument`；真正 TLS handshake/protocol/mbedTLS diagnostics → `tls_failure`。不要讓 generic `tls_error != ESP_OK` 的檢查搶在更具體 network/timeout evidence之前。
> 3. 對無法由現有 evidence 安全判定的情況採保守分類，不得自行推導 `scheme_mismatch`；classifier 永遠不得 invent HTTP compatibility fallback evidence。
> 4. 修正 `EspHttpTransport` PEM contract：non-null NUL-terminated PEM允許 `cert_len == 0`；若使用 nonzero length，必須符合 ESP-IDF v6.0.2 PEM contract並包含 NUL terminator。不得以 `strlen(pem)` 當 nonzero `cert_len` 而漏掉 `+1`。不得加入 insecure verify-off、trust-all 或 skip identity verification。
> 5. tests 必須直接覆蓋 representative timeout / DNS / socket-connect / TLS-handshake / certificate-identity / invalid-local-config evidence，確保 precedence 不互相誤分類。
>
> 驗證：targeted classifier/static tests → 既有 host regressions → ESP-IDF v6.0.2 C6 build → C3 build。不得執行 network、TLS handshake、flash、WF-RAC request。
>
> 若實際 ESP-IDF constants/semantics 與 pinned evidence不符，STOP 並回報 source/compile evidence；不要猜 mapping。
>
> 成功後從 `TASKS.md` 刪除整個 Stage C 區塊與所有 `→ Stage C` inventory 項目，`git diff --check` 後 commit/push。

### Stage D — Maintenance contracts / project rules

**推薦模型：** Luna  
**推理強度：** Low  
**推薦理由：** 以 deterministic maintenance、文件與小型 regression 為主，不需要高推理。  
**是否值得先用較便宜模型做前置蒐證：** 否；Luna Low 即為最低足夠配置。  
**Context 建議：** Default；只讀 operation-query builders/tests、frame/view public headers、`AGENTS.md`、`docs/architecture.md`、`TASKS.md`。  
**Execution mode 建議：** Normal / single-agent；不使用 Fast / Ultra / Multi-Agent。  
**依賴：** Stage A1–C PASS 且已自 `TASKS.md` 移除。  
**成功後：** 刪除本 Stage D 整段與 inventory 中 Stage D 項目；若只剩 Deferred 則保留 `TASKS.md`。

**Codex Prompt：**

> 執行「Stage D — Maintenance contracts / project rules」。這是一個低推理、機械式 maintenance task。遵守 `AGENTS.md`、`TASKS.md` 共通規則；先完成 remote-sync bootstrap，再只讀直接相關檔案，不要擴大 scope。
>
> 僅處理：
> 1. portable operation-query builders 所有 early failure 統一把 non-null `encoded_size` reset 為 0，並補 targeted regression；不得改 success encoding semantics。
> 2. 文件化 `AirconFrameView` / `AirconBlockView::segments` lifetime 與 immutability：views 指向 caller-owned raw storage；使用 returned views 期間 buffer 必須保持有效，validated bytes 不得在 cached CRC-valid state 背後被修改。
> 3. 修正 `docs/architecture.md` 把 `wfrac_transport_espidf` 說成「唯一」ESP-IDF adapter 的過時文字；目前至少有 `wfrac_transport_espidf` 與 `wfrac_time_espidf`。
>
> 不新增功能、不做重構。跑 operation-query targeted regression、必要 host regression、`git diff --check`。若只修改 docs/header contract且無 production source change，不為了形式預設跑 ESP-IDF full matrix。
>
> 成功後從 `TASKS.md` 刪除整個 Stage D 區塊與所有 `→ Stage D` inventory 項目；若只剩 Deferred 項目保留本檔。commit/push 正常進 `main`。

## Remaining task inventory

### P1 — Pre-hardware correctness

- Fix `decode()` remaining-capacity check so zero/exhausted logical output capacity cannot underflow and write before returning `output_too_small`; add sentinel tests. **→ Stage A1**
- Fix strict known-integer parsing so JSON whitespace before `result` / `apMode` remains legal after Stage A number refactor. **→ Stage A1**
- Refine ESP-IDF HTTP/TLS failure classification so DNS/socket/connect/connection-timeout evidence is not incorrectly collapsed into generic `tls_failure`; preserve the rule that the classifier never invents `scheme_mismatch`. **→ Stage C**
- Align `EspHttpTransport` PEM certificate length contract with ESP-IDF v6.0.2 semantics. Explicitly document/test `cert_len == 0` NUL-terminated PEM semantics and nonzero-length terminator requirements. **→ Stage C**

### P2 — Response parser / library hardening

- Compare known JSON keys and `command` by decoded string value, including semantic duplicate detection across escaped/raw spellings. **→ Stage B**
- Preserve and validate raw UTF-8 string bytes correctly instead of re-encoding each non-ASCII byte as a Unicode code point; validate strings even when skipped. **→ Stage B**

### P3 — Maintenance / contracts

- Reset `encoded_size` to zero consistently on early failure in portable operation-query builders. **→ Stage D**
- Document `AirconFrameView` / segment-view lifetime and immutability expectations: views reference caller-owned raw storage, and validated bytes must not be mutated behind cached CRC-valid state. **→ Stage D**
- Fix architecture wording that still describes `wfrac_transport_espidf` as the only ESP-IDF platform adapter now that `wfrac_time_espidf` also exists. **→ Stage D**

## Deferred

### Phase 1C #4H — Diagnostic Wi-Fi + read-only WF-RAC bootstrap firmware skeleton

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 開始涉及 ESP-IDF Wi-Fi lifecycle、bounded event handling、API/client/transport/time wiring 與 component integration；需要一般程式開發推理，但仍不需要 Sol。  
**是否值得先用較便宜模型做前置蒐證：** 通常否；A–D 完成後由 Terra 直接做 focused baseline read。只有 toolchain/baseline 是否仍可用不明時，才可先用 Luna Low 做純 evidence preflight，不修改 source。  
**Context 建議：** Default；只在 direct component interfaces 不足時升 Expanded，不預設 1M。  
**Execution mode 建議：** Normal / single-agent；不預設 Fast / Ultra / Multi-Agent。  
**觸發條件：** Stage A1–D 全部 PASS，尤其 P1 pre-hardware correctness 已清空。  
**來源註記：** 本 task 合併先前因 Codex 流量耗盡而未執行的完整 #4H Prompt；該舊 Prompt 的固定 baseline `f86d935...` 已過時，執行時一律以 remote-sync 後最新 `main` / `AGENTS.md` / `TASKS.md` 為 source of truth。  
**限制：** 只 IMPLEMENT / COMPILE / LINK。不得 flash、monitor、實際 connect Wi-Fi、HTTP/HTTPS、TLS handshake 或 WF-RAC；真正 hardware execution 留給下一個 read-only probe task。

**Codex Prompt：**

> 實作 Phase 1C #4H：standalone diagnostic Wi-Fi + read-only WF-RAC bootstrap firmware skeleton。先完成 remote-sync bootstrap，再讀 `AGENTS.md`、`TASKS.md`、`docs/references/README.md`，以及 topic-scoped 的 `api-envelope-discovery.md`、`timestamp-time.md`、`tls-transport.md`；只讀現有 API/client/transport/time public boundaries、`wfrac_transport_espidf`、`wfrac_time_espidf` 與 `firmware/idf_compile_smoke/` 的必要 CMake pattern。不要重新搜尋 upstream，不要讀 Device Model/Protocol/Matter internals，除非 compile dependency 真的要求。
>
> **Firmware positioning / tree：** 新增獨立 `firmware/wfrac_bootstrap_probe/` diagnostic harness，不是 production firmware、不是 future Matter Wi-Fi owner，也不是 reusable Wi-Fi library；不要建立 `components/wfrac_wifi_espidf`。合理拆分 project CMake、`Kconfig.projbuild`、`sdkconfig.defaults`、`main/CMakeLists.txt` 與 app/probe source，避免巨大單檔。沿既有 compile harness pattern，盡量讓 generated `sdkconfig` 留在 build directory，不在 repo root 產生可提交 sdkconfig。
>
> **Default inert / local config：** `CONFIG_WFRAC_PROBE_ENABLED=n` mandatory。disabled 時只記錄 `bootstrap probe disabled` 並停止，不能啟動 time seed、Wi-Fi 或 HTTP。Kconfig 只定義 fields、不放真值：probe enable、SSID、password、host、port（default 51443）、deviceId、operatorId、strict positive decimal int64 time seed、explicit HTTP/HTTPS scheme，以及 caller-supplied HTTPS trust-certificate boundary。不得提交或 log real SSID/password/operatorId/endpoint/cert/private data；不要完整 dump request JSON。Wi-Fi password可 empty 以支援 open AP，但不得推定實際網路為 open。
>
> **Enabled config gate：** 在任何 Wi-Fi operation 前驗證 SSID/host/deviceId/operatorId non-empty、port > 0、time seed strict positive int64。invalid config → generic FAIL/STOP，不得 connect。使用 `EspSystemTimeProvider` 以 `TimeQuality::estimated` seed；不得 SNTP/NTP/DNS time/build timestamp，seed failure → STOP。
>
> **Wi-Fi lifecycle：** 採 ESP-IDF v6.0.2 normal STA flow：`esp_netif_init()` → default event loop → default STA netif → `esp_wifi_init()` → `esp_wifi_set_storage(WIFI_STORAGE_RAM)` → STA mode/config → start → **每 boot 最多呼叫 `esp_wifi_connect()` 一次**。註冊 `WIFI_EVENT_STA_DISCONNECTED` 與 `IP_EVENT_STA_GOT_IP`；handler 只記錄 bit/result/disconnect reason，不得在 disconnected handler reconnect。使用 EventGroup 或等價最小同步，bounded wait 以 **20 秒**為預設合理值；disconnected 或 timeout 都停止，不進 HTTP。只有 `IP_EVENT_STA_GOT_IP` 可進 request path，STA_CONNECTED 不足。禁止 AP scan、SmartConfig、SoftAP、BLE provisioning、自動 reconnect/retry loop。
>
> **Persistence / PHY：** Wi-Fi config RAM-only，禁止 `WIFI_STORAGE_FLASH`。`sdkconfig.defaults` 設 `CONFIG_ESP_PHY_CALIBRATION_AND_DATA_STORAGE=n` 並維持 `CONFIG_ESP_PHY_INIT_DATA_IN_PARTITION=n`；若 C6/C3 的 v6.0.2 option實際不同，只依 compile evidence做最小調整。Project code不得 `nvs_flash_init()` / `nvs_open()` / `nvs_set_*()` / `nvs_commit()`，不得新增 Operator ID、scheme、credential persistence。若實際 build/runtime證明底層 NVS init不可避免，STOP 回報 evidence，不自行擴 scope。
>
> **Scheme / TLS：** scheme 必須由 config explicit。HTTP 用 `SchemeKnowledge::http_confirmed`，HTTPS 用 `https_confirmed`；這只是 explicit diagnostic config，不是 discovered/persisted evidence。不得建立 unknown cache觸發 fallback；每 boot WF-RAC request **maximum = 1**，TLS/timeout/connect failure也不得改試另一 scheme。HTTPS 僅在 caller-supplied verified trust cert存在時才允許 request；缺 cert時可在 Wi-Fi GOT_IP 後、HTTP 前明確記錄 `HTTPS trust certificate missing` 並 STOP。保留 `server_cert_pem` + size boundary，遵守 Stage C 完成後的最新 cert-length contract；禁止 verify-off、trust-all、skip-CN/insecure escape hatch。
>
> **Read-only request wiring：** 只 wiring `EspSystemTimeProvider` → `build_get_device_info_request()` → `execute_api_request()` → `EspHttpTransport` → `parse_api_response_envelope(..., "getDeviceInfo")` → `parse_get_device_info_contents()`。`ApiClientContext.api_version = "1.0"`，使用 caller-owned bounded fixed buffers（request約 512–1024、HTTP response約 2048、identity buffers合理固定大小）；project source不得 explicit `new`/`malloc`/`std::string`/`std::vector`。唯一 WF-RAC command 是 `getDeviceInfo`；不得 call/build/send `getAirconStat`、`setAirconStat`、`updateAccountInfo`、`deleteAccountInfo`、registration/polling/second command。
>
> **Response semantics：** transport若提供完整 body，即使 HTTP status非 2xx也允許嘗試 parse API envelope，以保留 error-body evidence。Envelope成功可記 command parse PASS 與 raw result integer；不得替 `result=2` 或任何非零值發明 global meaning。若 contents存在才 parse getDeviceInfo；可 bounded log `airconId`、optional MAC/apMode，但不要持久化 identity，也不要洩漏 request secrets。
>
> **Stages / cleanup：** diagnostic logs 至少能區分 `BOOT / CONFIG / TIME / WIFI_INIT / WIFI_CONNECT / GOT_IP / TRANSPORT / HTTP_STATUS / API_ENVELOPE / DEVICE_INFO / CLEANUP` 的 PASS/FAIL，不把既有 Transport/API errors flatten成單一 bool。one-shot完成或失敗後合理 unregister handlers、connected時才 disconnect、stop/deinit Wi-Fi、destroy default STA netif、delete EventGroup；benign already-stopped cleanup不得掩蓋 primary result，不得 restart probe。
>
> **Compile-only hard gate：** 本 #4H 的 actual Wi-Fi executions = 0、HTTP/TLS = 0、WF-RAC = 0、flash = 0、monitor = 0。新 firmware使用獨立 build dirs，ESP32-C6 compile/link PASS + ESP32-C3 compile/link PASS，不共用 build cache。跑既有 Python validator/tests與 host Protocol/Device Model/Transport/API request/API response/Client/Read Path regressions；若沒有修改既有 `idf_compile_smoke`/shared platform component，不為了重複證明再跑額外同內容 matrix。不得新增需要 Wi-Fi secrets的 GitHub CI。
>
> **Docs / AGENTS / evidence：** 最小更新 README/architecture/roadmap與 `AGENTS.md`，只讓規則反映「允許 diagnostic ESP-IDF Wi-Fi source、RAM-only one-shot bounded connect、read-only getDeviceInfo wiring、compile/link validation」，但仍禁止本 task 實際 network/flash/monitor、mDNS、app NVS persistence、registration、writes、polling、Matter。文件明確標記 `COMPILE-READY`，不是 `HARDWARE-VALIDATED`；application persistence = not implemented、probe Wi-Fi config = RAM-only、PHY calibration persistence = disabled for probe。Evidence最高只能 Software/Compile，`CONFIRMED_LOCAL` 維持 0，library readiness 必須 `PRESERVED`。
>
> **Cleanup / validation / commit：** 移除 build-probe-c6/build-probe-c3、ELF/BIN/MAP/CMake cache/temp artifacts，確認無 sdkconfig secrets進 working tree；`git diff --check` PASS。所有 gates PASS 後 commit `feat: add diagnostic WF-RAC bootstrap firmware`，normal push `origin/main`。成功後從 `TASKS.md` 刪除整個 #4H Deferred task，但保留下一個 `First real WF-RAC read-only probe / hardware validation`；不要自行執行它。Final report沿共通精簡格式，另外只補：probe default enabled/disabled、C6/C3 build、actual network/flash execution counts、library readiness、`CONFIRMED_LOCAL`。

### First real WF-RAC read-only probe / hardware validation

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 需要把實機 Wi-Fi/TLS/WF-RAC evidence與既有 classifier/parser 語意整合判讀；屬 integration/debug，不適合 Luna 自行決策，也不需 Sol。  
**是否值得先用較便宜模型做前置蒐證：** 若只是確認 build/toolchain/dirty tree，可用 Luna Low；實際 probe evidence interpretation 仍由 Terra。  
**Context 建議：** Default；先只讀 diagnostic firmware、transport classifier、getDeviceInfo parser、相關 reference topic；evidence不足再 Expanded。  
**Execution mode 建議：** Normal / single-agent；硬體測試不使用 Multi-Agent。  
**觸發條件：** #4H compile-ready，local endpoint/scheme/time seed/TLS trust inputs 明確，使用者明確授權 hardware/network execution。  
**限制：** bounded、read-only、`getDeviceInfo` first；不得 insecure TLS、不得自動 downgrade、不得寫入控制命令。任何真實 credential/operator/cert material不得 commit或輸出。

**Prompt scaffold（硬體資料由使用者當次提供，執行前刷新）：**

> 執行第一次 real WF-RAC read-only probe。先完成 remote-sync / Git/toolchain preflight，不修改 production source，除非 evidence 顯示 `CONFIRMED ROOT CAUSE` 且使用者當次明確授權修補。只使用 diagnostic firmware 已存在的 bounded one-shot path。
>
> 先確認 explicit scheme、host/port、time seed與 HTTPS trust input；不得 auto-detect/downgrade，不得 verify-off。Flash/run 後只執行單次 Wi-Fi connect與單次 `getDeviceInfo` request，收集最小必要 serial evidence；不得 registration、setAirconStat、polling或任何控制寫入。
>
> 將結果明確分成 Software/Compile/Static-Test/Bench/Hardware/Network evidence；API/transport call accepted 不等於 WF-RAC application success。若失敗，依 classifier + response evidence分類 root cause；同一 root cause 最多重試一次。需要 source patch但證據不足時 STOP，不猜。
>
> 成功時只把可驗證結果更新到正式 evidence/docs（依當次使用者授權）並同步更新 `TASKS.md`；不得提交 secrets。若 `getDeviceInfo` 真實成功，可將對應項目標為完成並規劃下一個 read-only `getAirconStat` task，但不得自行執行未授權下一階段。
