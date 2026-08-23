# Unfinished work queue

此檔只追蹤尚未完成、Blocked、Deferred 或待驗證工作。完成項目應直接移除；Git history 才是完成紀錄。不得因看到本檔而自行執行未明確授權的工作。

執行通則：每次 Codex 只執行當次明確授權的 stage；開始時先讀 `AGENTS.md`、本 `TASKS.md` 與該 stage 直接相關檔案，不得因本佇列存在就自行處理其他項目。成功驗證後，移除或更新本檔對應項目；若無未完成工作則刪除 `TASKS.md`。若 Luna 在已指定 root cause / patch direction 下仍無法安全完成，保留證據並升級 Terra；不得無證據直接升 Sol。

## Codex staged execution plan — 流量恢復後依序執行

### Stage A — Parser memory/correctness hardening

**推薦模型：** Luna  
**推理強度：** Medium  
**前置蒐證：** 不需要；root causes 已由 code review 定位。  
**依賴：** 無；第一個執行。  
**成功後：** 更新本 `TASKS.md`，移除本 stage 已完成項目，再 commit/push。

**Codex Prompt：**

> 在目前 `main` 上執行「Stage A — Parser memory/correctness hardening」。先讀 `AGENTS.md`、`TASKS.md`、`components/wfrac_api/src/api_response.cpp`、public response header 與直接相關 host tests；不要做 repo-wide 掃描。已知 root causes 已確認，不要重新設計 parser architecture。
>
> 僅處理以下 scope：
> 1. 修正 response string `decode()`：empty string / zero-capacity / null caller output 不得造成 null write；所有 failure path 必須安全且 deterministic。
> 2. 將 JSON/Unicode parser 內可能超過 one-past-end 的 `p + N` bounds checks 改為 remaining-length 型式；覆蓋 truncated `\uXXXX`、high-surrogate 缺 low-surrogate、尾端 escape。
> 3. `parse_api_response_envelope()` 與 `parse_get_device_info_contents()` 改成 failure-atomic output：只在完整成功後 commit metadata；失敗時 output 維持 reset 狀態。
> 4. identity/C-string outputs（至少 `airconId`、`macAddress`）不得接受 decoded embedded NUL；維持 public C-string contract 無歧義。
> 5. unknown JSON numeric values新增完整合法 JSON number skip（含 fraction/exponent），但 known `result` / `apMode` 仍必須 strict integer。
>
> 新增 targeted host tests，至少覆蓋上述每個 bug；不得改協議語意、Transport、Device Model、ESP-IDF runtime、Wi-Fi、NVS、Matter。先跑 API response targeted tests，再跑既有 host regressions；只有相關 source 變動需要的驗證才擴張。若 root cause 與實際 source 不符或修法需要改 public architecture，STOP 並回報，不要猜。
>
> 全部 PASS 後同步更新 `TASKS.md`，移除 Stage A 對應未完成項目；若仍有其他工作保留本檔。commit/push 正常進 `main`，禁止 force/reset/history rewrite。

### Stage B — JSON string semantics / UTF-8 hardening

**推薦模型：** Luna  
**推理強度：** Medium  
**前置蒐證：** 不需要；只依 JSON/UTF-8 標準語意與現有 parser contract。  
**依賴：** Stage A PASS。  
**升級條件：** 若 raw UTF-8 validation 需要明顯擴大 parser architecture，停止並升 Terra Medium。  
**成功後：** 更新 `TASKS.md`，再 commit/push。

**Codex Prompt：**

> 執行「Stage B — JSON string semantics / UTF-8 hardening」。必須以 Stage A 已完成的 `main` 為 baseline。先讀 `AGENTS.md`、`TASKS.md`、目前 `api_response.cpp` 與其 host tests，只讀直接相關檔案。
>
> 僅處理：
> 1. known JSON keys 與 top-level `command` 必須以 decoded JSON string semantics 比較，使合法 escape（例如 `\uXXXX`）與等價 raw spelling 得到相同語意；不要放寬 duplicate-key 或 strict known-field type rules。
> 2. raw non-ASCII UTF-8 bytes不得被逐 byte 當 Unicode code point 再編碼；合法 raw UTF-8應 byte-preserving，`\uXXXX` 則正確輸出 UTF-8。
> 3. 驗證並拒絕 malformed UTF-8，包括 bad continuation、truncated sequence、overlong encoding、UTF-8 encoded surrogate、超過 U+10FFFF；不得接受 embedded U+0000 identity（Stage A contract保持）。
> 4. 加 targeted tests：raw `é`、中文字串 byte-identical；`\u00E9` 等價 decode；escaped key/command；malformed UTF-8 cases。
>
> 不得引入 heap/external JSON library，不得重寫整個 parser，不得碰 Transport/Device Model/ESP-IDF runtime。targeted API response tests PASS 後跑既有 host regressions。若為完成此 scope 必須改 public API 或大規模 parser rewrite，STOP 並保留 evidence，建議 Terra Medium。
>
> 成功後同步移除 `TASKS.md` 的 Stage B 對應項目，再 commit/push。

### Stage C — ESP-IDF transport diagnostics correctness

**推薦模型：** Luna  
**推理強度：** Medium  
**前置蒐證：** 不需要重新搜尋；以 ESP-IDF v6.0.2 官方既有 evidence 與目前 adapter source 為準。  
**依賴：** Stage A PASS；可在 Stage B 之後執行以維持線性 history。  
**成功後：** 更新 `TASKS.md`，再 commit/push。

**Codex Prompt：**

> 執行「Stage C — ESP-IDF transport diagnostics correctness」。先讀 `AGENTS.md`、`TASKS.md`、`wfrac_transport_espidf` classifier / `EspHttpTransport` 及直接相關 compile-smoke/tests；不要重新研究整個 repo。
>
> 已知問題與要求：
> 1. error classifier 不得把 ESP-TLS 層回報的 DNS/socket/connect/connection-timeout evidence 一律壓成 generic `tls_failure`。依 ESP-IDF v6.0.2 error semantics，把可明確辨識的 timeout → `timeout`、DNS/socket/connect/network reachability → `connection_failure`、certificate verify flags → `tls_identity_failure`、真正 TLS handshake/protocol diagnostics → `tls_failure`；local invalid/config transport → `invalid_argument`。未知證據保守落 `io_failure` 或既有最合理 taxonomy。classifier 永遠不得自行產生 `scheme_mismatch`。
> 2. 修正 `EspHttpTransport` server PEM length contract 與 ESP-IDF v6.0.2：清楚支援/文件化 `cert_len == 0` 代表 NUL-terminated PEM；非零 length 的契約必須與 ESP-IDF 要求一致（包含 terminator）。不得加入 insecure verify-off、trust-all、skip identity verification。
> 3. 加 compile-time / targeted classifier coverage，確保 DNS/connect/timeout/TLS/identity 不互相誤分類；更新必要的 adapter comments/docs，但不要碰 Wi-Fi/runtime。
>
> 驗證：targeted classifier/static tests、既有 host regressions，以及 ESP-IDF v6.0.2 C6/C3 compile-link。不得執行 network、TLS handshake、flash、WF-RAC request。若 ESP-IDF public constants/semantics與既有 evidence不符，STOP 回報來源與 compile evidence，不要猜 mapping。
>
> 成功後同步更新 `TASKS.md`，移除 Stage C 對應項目，再 commit/push。

### Stage D — Maintenance contracts / project rules

**推薦模型：** Luna  
**推理強度：** Low  
**前置蒐證：** 不需要。  
**依賴：** Stage A–C PASS。  
**成功後：** 更新 `TASKS.md`；若只剩 Deferred 項目則保留，若沒有任何未完成工作才刪除。

**Codex Prompt：**

> 執行「Stage D — Maintenance contracts / project rules」。這是一個低推理、機械式 maintenance task。先讀 `AGENTS.md`、`TASKS.md` 與直接相關檔案，不要擴大 scope。
>
> 僅處理：
> 1. portable operation-query builders 所有 early failure 統一把 `encoded_size` reset 為 0，並補 targeted regression。
> 2. 文件化 `AirconFrameView` / `AirconBlockView::segments` lifetime 與 immutability：views 指向 caller-owned raw storage；使用 returned views 期間 buffer 必須保持有效，validated bytes 不得在 cached CRC-valid state 背後被修改。
> 3. 在 `AGENTS.md` 加永久規則：`If TASKS.md exists at the repository root, read it near the beginning of the task. Treat it as the shared temporary queue of unfinished work. After successfully completing and validating an authorized task, remove or update the corresponding entry. Do not execute unrelated TASKS.md items without explicit user authorization. Delete TASKS.md when no unfinished work remains.`
> 4. 在 `AGENTS.md` 加 library-extraction invariant：目前維持 monorepo；portable core `wfrac_protocol` / `wfrac_device_model` / `wfrac_api` / `wfrac_transport` / `wfrac_client` 必須保持未來可低成本抽成獨立 C++ library；platform/runtime dependency 只能 inward（platform/runtime → portable core），portable public API 不得洩漏 ESP-IDF/FreeRTOS/Wi-Fi/NVS/Matter types；不得因此提前拆 repo、packaging、semantic versioning。
> 5. 修正 architecture 文件把 `wfrac_transport_espidf` 說成「唯一」ESP-IDF adapter 的過時文字；目前至少有 `wfrac_transport_espidf` 與 `wfrac_time_espidf`。
>
> 不新增功能、不做重構。跑必要 targeted tests、`git diff --check`；若 source 有變才跑其相關 host regression。成功後同步更新 `TASKS.md`，移除 Stage D 對應項目並 commit/push。

## Remaining task inventory

### P1 — Pre-hardware correctness

- Fix `wfrac_api` response string decoding so empty strings / zero-capacity caller outputs cannot cause a null write. Add targeted malformed/empty-output tests. **→ Stage A**
- Replace unsafe end-of-buffer pointer-addition checks in JSON/Unicode parsing with remaining-length checks; add truncated `\uXXXX` / surrogate-pair tests. **→ Stage A**
- Refine ESP-IDF HTTP/TLS failure classification so DNS/socket/connect/connection-timeout evidence is not incorrectly collapsed into generic `tls_failure`; preserve the rule that the classifier never invents `scheme_mismatch`. **→ Stage C**
- Align `EspHttpTransport` PEM certificate length contract with ESP-IDF v6.0.2 semantics. Explicitly document/test whether `cert_len == 0` means NUL-terminated PEM and how nonzero lengths include the terminator. **→ Stage C**
- Make response-parser outputs atomic on failure: `parse_api_response_envelope()` and `parse_get_device_info_contents()` should commit metadata only after full success, matching the `getAirconStat` parser behavior. **→ Stage A**

### P2 — Response parser / library hardening

- Reject decoded embedded NUL in identity/C-string outputs such as `airconId` / `macAddress`, or otherwise make the public contract unambiguous. **→ Stage A**
- Allow unknown JSON numeric values to be safely skipped using full valid JSON number grammar while keeping known integer fields such as `result` / `apMode` strict integers. **→ Stage A**
- Compare known JSON keys and `command` by decoded string value so valid escapes such as `\uXXXX` do not break semantic matching. **→ Stage B**
- Preserve and validate raw UTF-8 string bytes correctly instead of re-encoding each non-ASCII byte as a Unicode code point; add valid and malformed UTF-8 tests. **→ Stage B**

### P3 — Maintenance / contracts

- Reset `encoded_size` to zero consistently on early failure in portable operation-query builders. **→ Stage D**
- Document `AirconFrameView` / segment-view lifetime and immutability expectations: views reference caller-owned raw storage, and validated bytes must not be mutated behind cached CRC-valid state. **→ Stage D**
- Add the permanent `TASKS.md` queue rule to `AGENTS.md`. **→ Stage D**
- Add the library-extraction invariant to `AGENTS.md`: keep the monorepo for now, preserve portable-core extractability, and prevent ESP-IDF/FreeRTOS/Wi-Fi/NVS/Matter types from leaking into portable public APIs. **→ Stage D**
- Fix architecture wording that still describes `wfrac_transport_espidf` as the only ESP-IDF platform adapter now that `wfrac_time_espidf` also exists. **→ Stage D**

## Deferred

### Phase 1C #4H — Diagnostic Wi-Fi + read-only WF-RAC bootstrap firmware skeleton

**推薦模型：** Terra  
**推理強度：** Medium  
**觸發條件：** Stage A–D 全部 PASS，尤其 P1 pre-hardware correctness 已清空。  
**原因：** 開始涉及 ESP-IDF Wi-Fi lifecycle、bounded event handling、existing API/client/transport/time wiring；不適合再當純機械 patch。  
**限制：** 先做 compile/link-ready one-shot diagnostic firmware；預設 inert；不得 flash/network/real WF-RAC。具體 Prompt 應在執行前依當時 `main` / `AGENTS.md` / `TASKS.md` 重新生成，不在本檔鎖死可能已變動的 source 細節。

### First real WF-RAC read-only probe / hardware validation

**推薦模型：** Terra  
**推理強度：** Medium  
**觸發條件：** #4H compile-ready，local endpoint/scheme/time seed/TLS trust inputs 明確。  
**限制：** bounded、read-only、`getDeviceInfo` first；不得 insecure TLS、不得自動 downgrade、不得寫入控制命令。具體 hardware Prompt 在執行前依當時 source of truth 重新生成。
