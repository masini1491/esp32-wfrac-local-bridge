# Unfinished work queue

此檔只追蹤尚未完成、Blocked、Deferred 或待驗證工作。完成項目應直接移除；Git history 才是完成紀錄。不得因看到本檔而自行執行未明確授權的工作。

本 repository 採 persistent `TASKS.md` mode；queue 清空時保留此固定 coordination surface，並依 common playbook 收斂為最低充分 `EMPTY` state，不刪除檔案。

## Codex queue / launch rules

- 每次只執行使用者當次明確授權的 Stage；`TASKS.md` 本身不授權開始其他項目。
- 共通的 repository identity、workspace / permission / credential capability、remote sync、Git safety、failure taxonomy / retry、validation / evidence、Completion Evidence Guard、Progressive Repository Reading、Model / Reasoning / Context / Agent / cost control、long-running operation 與 reporting timestamp，均以最新 `AGENTS.md` routing 到 `masini1491/ai-development-playbook` 的規則為 authority；本檔不複製完整 common policy，避免 drift。
- `TASKS.md` 只收錄需要未來追蹤或再次執行的 TODO / Blocked / Deferred / Pending-validation 工作。一次性、已知位置、已知修改內容或 root cause、低風險且完成後無追蹤價值的小型 maintenance，不必為形式建立 queue item。
- 本檔的推薦模型、推理強度、Context、Execution mode 只是給使用者的 launch 建議；Codex 不得自行升降模型、推理強度、Context、Fast / Ultra 或 Multi-Agent。若 evidence 顯示目前設定不足，STOP、保留 evidence，由使用者重新 launch。
- 執行 Stage 前先安全同步最新 `origin/main`，完成 Repository Identity / workspace capability / Git state / permission gates；只允許 clean、expected `main`、無 local ahead/diverged 且可 fast-forward-only 的同步。同步後重新讀最新 `AGENTS.md` / `TASKS.md`；若 Stage 已消失或 scope 已改變，STOP，不得依舊 Prompt 或記憶執行。
- Repository reading 從目前 Stage 的最低充分 evidence 開始，依 routed Progressive Repository Reading 逐級擴張；禁止為熟悉專案預設 repo-wide 掃描。
- 不得因執行途中發現其他問題就順便跨 Stage、architecture、runtime、hardware、deployment 或其他 mutation boundary；out-of-scope work 留待新 task / Stage 或 STOP 回報。
- Stage 宣稱完成、更新 validation state、移除 queue item、commit / push 或進入下一 Stage 前，必須依 Completion Evidence Guard，以最低充分 canonical repository evidence 交叉確認 current HEAD / scoped diff / required validation / `TASKS.md` state。自然語言 completion report 不得覆蓋 repository evidence。
- 成功且證據完整後刪除已完成 Stage，不保留 Completed 區段；queue lifecycle 依最新 `AGENTS.md` routing 的 common playbook authority，完全無 unfinished item 時收斂為 `EMPTY` state。

### Codex 短指令啟動格式

使用者可在 Codex UI 手動選好該 Stage 指定的模型與推理強度後，只貼短指令：

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP；必要 operation 若缺權限，依最新 `AGENTS.md` routing 的 common playbook 要求最小必要 capability，不得自行繞過）。同步後讀根目錄 `AGENTS.md` 與 `TASKS.md`，依 `AGENTS.md` routing 先啟用 `CODEX_EXECUTION.md` 的 always-on Codex reporting contract，再只讀本 Stage 所需的其他最低必要 Playbook / project evidence，完整執行 `TASKS.md` 的「<Stage 名稱>」，只執行該 Stage。完成前依 Completion Evidence Guard 交叉確認 canonical repository / validation / queue state；成功後依 queue lifecycle 更新 `TASKS.md`、commit / push（僅限本次 launch 已授權者）。

## Ready

### Diagnostic Web Test Page for read-only bootstrap probe

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 此 Stage 會在 ESP-IDF diagnostic firmware 內加入 local HTTP/UI surface，需同時處理既有 Wi-Fi/probe lifecycle、bounded operation、secret boundary 與 C6/C3 compile compatibility；屬中等複雜度 embedded integration，不需 Sol。  
**是否值得先用較便宜模型做前置蒐證：** 否；目前 `wfrac_bootstrap_probe`、Kconfig、read-only `getDeviceInfo` path 與 project boundary 已知，Terra Medium 可直接用最低充分 context implementation。  
**Context 建議：** L0 → L3；先讀最新 `AGENTS.md` / `TASKS.md`，再讀 `firmware/wfrac_bootstrap_probe/`、直接使用的 transport/client/time interfaces、`docs/security.md`，並依 routing 只取得本 Stage 必要的 `EMBEDDED_PROJECTS.md`、`UI_UX.md`、`DEBUG_VALIDATION.md` / toolchain authority；禁止 repo-wide loading。  
**Execution mode 建議：** Normal / Agent 1；不使用 Multi-Agent。  
**Admission reason：** 新到貨 ESP32-C3 SuperMini 使實機 bring-up 即將開始；目前 probe 只能靠 Kconfig + serial 操作。加入 browser-driven diagnostic surface 會新增 firmware runtime/UI boundary，且後續需接 hardware validation，具有 material implementation / validation tracking value，因此應保存為 canonical Stage，而不是 Direct Short Prompt。  
**本次明確授權例外：** 使用者於 2026-09-02 明確同意把「C3 實機測試用網頁」插入 Phase 1C。這只構成目前 `AGENTS.md`「WebUI 未經另行授權不得實作」規則的**窄範圍 diagnostic exception**：只允許 `wfrac_bootstrap_probe` 的測試頁，不授權正式 Provisioning / Product WebUI、Matter UI、MQTT UI、OTA UI 或其他 runtime scope expansion。

**目標 contract：**
- 在既有 `firmware/wfrac_bootstrap_probe` 內加入最小、自包含、local-only 的 diagnostic web test page；它是 test consumer / operator surface，不是 production provisioning UI。
- 預設仍為 inert；只有 diagnostic probe 已明確 enable 且使用者主動操作時才可執行測試。頁面載入本身不得自動送 WF-RAC request。
- 本 Stage **不做 SoftAP / captive portal / Wi-Fi provisioning**。先沿用既有 Kconfig station Wi-Fi bootstrap，使 ESP32 加入測試 LAN 後提供頁面；Wi-Fi SSID/password 不經此 HTTP page 傳輸。
- `Operator ID` 維持既有 local Kconfig / RAM-only diagnostic input，不得在頁面顯示、回填或透過 plaintext page 收集；UI 只可顯示「已設定／未設定」等非 secret readiness。其他 project-defined secret 同樣 write-only / non-echo。
- 頁面可提供本次 read-only probe 所需的非 secret / public test inputs，例如 WF-RAC host、port、device ID、explicit HTTP/HTTPS scheme、estimated time seed、server trust certificate PEM；若 current security contract 將其中任何欄位視為 sensitive，依較嚴格 contract 收斂，不得為 UI 便利放寬。
- 提供單一明確 action 執行**一次** read-only `getDeviceInfo`；不得 registration、`setAirconStat`、控制寫入、polling、自動重試 loop 或其他 API action。
- 一次只允許一個 active probe；重複 submit 必須 bounded reject/disable，並保留 timeout / failure / unknown 的可判讀結果，不能只用「request 已送出」顯示成功。
- 顯示最小充分 sanitized 結果：至少區分 input validation、Wi-Fi/network readiness、transport/TLS/HTTP、WF-RAC application result 與 parser/result outcome；不得把 raw secret、Authorization material、Wi-Fi password、Operator ID 或不必要的 secret-derived characteristic輸出到 HTML、JSON、serial/log 或 final report。
- UI assets 必須 self-contained，不依賴 CDN、remote font、Internet 或 cloud；保持 resource-light，優先 vanilla HTML/CSS/必要的最小 JS，不為測試頁導入大型 frontend framework。
- UI 只 presentation / interaction；不得藉此改變 portable protocol、Device Model、wire contract、scheme fallback、TLS identity semantics、time-quality semantics 或 persistence ownership。若 integration 揭露上述 machine contract 需要 semantic change，保存 evidence 後 STOP，另開 scoped task。
- 不新增 NVS/persistence、mDNS、Matter、MQTT、OTA、production authentication/provisioning、正式 product navigation 或完整 settings architecture。

**Validation / evidence：**
- 保留既有 host validation PASS；若新增可抽離的 form/API parsing 或 validation helper，加入最低充分 deterministic targeted tests。
- `wfrac_bootstrap_probe` 對 ESP32-C6 primary 與 ESP32-C3 compatibility target 都必須 compile/link，並確認 intended HTTP server/UI source 真正進入 build，不得以被條件編譯排除的假 PASS 代替。
- 靜態檢查 default-inert、single active probe、read-only `getDeviceInfo` boundary、no external assets、no persistence，以及 secret non-echo/non-log invariant。
- 本 Stage **不執行真實 Wi-Fi / HTTP / TLS / WF-RAC hardware/network test**；compile/static PASS 只能把網頁測試 harness 提升到 `COMPILE-READY`，不得宣稱 Hardware/Network PASS。

**Queue relation / completion gate：** 成功後 browser-driven diagnostic harness 應達 `COMPILE-READY`，C6/C3 compile/link 與 targeted validation PASS，且不削弱既有 read-only/security boundary；完成後移除此 Stage。後續真實 C3 board + WF-RAC 測試仍由既有「First real WF-RAC read-only probe / hardware validation」Stage 管理，不得在本 Stage 順手執行硬體測試。

## Deferred

### Machine-readable TimeProvider implementation-state reconciliation

**推薦模型：** Luna  
**推理強度：** Low  
**推薦理由：** current drift、直接 source 與 validator location 均已確認；工作重點是精確區分「portable TimeProvider boundary 已實作」與「production time acquisition/persistence/runtime 尚未完成」，屬 bounded contract/test maintenance。  
**是否值得先用較便宜模型做前置蒐證：** 否；Luna Low 已是最低充分設定。  
**Context 建議：** L0 → L2；只讀 `specs/wf-rac-contracts.json` 的 R5、`tools/validate_contracts.py` 對應 checker/tests、portable `time_provider.hpp`、ESP-IDF time adapter與 current roadmap/topic。  
**Execution mode 建議：** Normal / Agent 1。  
**Admission reason：** machine-readable contract 仍將 `future_time_provider.exact_runtime_enum` 標為 `PENDING` 且 `implementation=false`，validator 也強制此舊狀態，但 current source 已有 `TimeQuality { unavailable, estimated, trusted }`、`ITimeProvider` 與 ESP-IDF provider。這會使 validation PASS 無法代表 machine-readable implementation-state 與 current source 一致，屬 material contract/validation drift。  
**限制：** 不得把尚未存在的 trusted-time acquisition、persistence、SNTP/NTP dependency、power-loss recovery或 production runtime宣稱為已完成；只校正 machine-readable implementation-state vocabulary 與相應 validator/tests/docs consistency。  
**Completion gate：** contract 能明確表示 portable provider/time-quality boundary 已實作，同時保留 actual time acquisition/persistence/runtime 的未完成狀態；validator/tests 對新語意有 targeted coverage，既有 host validation PASS。成功後移除此項。

### Continuous ESP-IDF C6/C3 compile/link CI gate

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 涉及 GitHub Actions、ESP-IDF v6.0.2 toolchain provisioning/caching、matrix scope 與 validation cost，需要兼顧可重現性與 CI 成本，但不需要 Sol。  
**是否值得先用較便宜模型做前置蒐證：** 可；若只盤點 current workflow 與官方 ESP-IDF CI 安裝方式，可先用 Luna Low 唯讀蒐證，完成後 STOP 由使用者決定正式 implementation launch。  
**Context 建議：** L0 → L3；先讀 workflow、兩個 firmware CMake、adapter CMake 與 toolchain contract，不做 repo-wide loading。  
**Execution mode 建議：** Normal / Agent 1。  
**Admission reason：** current GitHub Actions 只覆蓋 machine contract、Python host tests 與 portable C++ host tests；ESP-IDF-specific transport/time adapters、`idf_compile_smoke`、`wfrac_bootstrap_probe` 與 C6/C3 portability 沒有 continuous CI regression gate。這是持續性的 platform/validation coverage，而非一次性小修。  
**限制：** 只建立 compile/link CI；default-inert，不得執行 hardware、Wi-Fi、HTTP/HTTPS、TLS handshake 或 real WF-RAC；不得把 credential/operator/cert material放入 workflow。CI 成本需 bounded，避免無必要 matrix/context expansion。  
**Completion gate：** current supported ESP-IDF baseline 下，至少能自動 compile/link platform integration 與 diagnostic firmware 的 ESP32-C6 primary、ESP32-C3 compatibility targets；failure phase 可區分 toolchain/config/source；既有 host validation 保持 PASS。成功後移除此項。

### First real WF-RAC read-only probe / hardware validation

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 需要把實機 Wi-Fi/TLS/WF-RAC evidence 與既有 classifier/parser 語意整合判讀；屬 hardware-facing integration/debug，不適合 Luna 自行決策，也不需 Sol。  
**是否值得先用較便宜模型做前置蒐證：** 若只是確認 build/toolchain/Git state，可用 Luna Low；實際 probe evidence interpretation 仍由 Terra。  
**Context 建議：** L0 → L2；先讀 Git/AGENTS/TASKS/current evidence，再讀 diagnostic firmware、transport classifier、getDeviceInfo parser 與直接 caller/dependency；只有能明確說出缺少哪個答案時才升 L3，禁止預設 L4/L5。  
**Execution mode 建議：** Hardware evidence / Agent 1；不使用 Multi-Agent。  
**觸發條件：** #4H diagnostic harness compile-ready；當次測試 target/board/FQBN/device identity 明確；local endpoint/scheme/time seed/TLS trust inputs 明確；使用者明確授權本次 hardware/network execution。
**限制：** bounded、read-only、`getDeviceInfo` first；不得 insecure TLS、不得自動 downgrade、不得寫入控制命令。任何真實 credential/operator/cert material不得 commit、log、serial output 或 final report。Hardware/network execution authorization 不自動包含 repository mutation、commit 或 push authorization。

**Prompt scaffold（硬體資料由使用者當次提供，執行前刷新）：**

> 執行第一次 real WF-RAC read-only probe。先完成 remote-sync / Git/toolchain preflight，確認 exact target/board/FQBN/device identity 與本次必要 execution permission。只使用 diagnostic firmware 已存在的 bounded one-shot path。本 Stage 不修改 production source；若 evidence 指向 `SOURCE` 且 root cause 達 `CONFIRMED ROOT CAUSE` 或 `HIGH-CONFIDENCE LIKELY ROOT CAUSE`，保存最低充分 evidence 後 STOP，另開 Focused Patch Stage，不得在 hardware probe 中順手修補。
>
> 先確認 explicit scheme、host/port、time seed 與 HTTPS trust input；不得 auto-detect/downgrade，不得 verify-off。Flash/run 後只執行單次 Wi-Fi connect 與單次 `getDeviceInfo` request，收集最小必要 serial evidence；serial/network observation 使用 bounded monitor window，若無新增 progress evidence則保存目前狀態並 STOP。不得 registration、setAirconStat、polling或任何控制寫入。
>
> 將結果明確分成 Software/Compile/Static-Test/Bench/Hardware/Network evidence；API/transport call accepted 不等於 WF-RAC application success。Hardware/Network evidence只對本次實際 target/board/device/WF-RAC/network environment與已執行的 read-only `getDeviceInfo` path成立，不得外推到另一 board/FQBN、write path、polling、Matter、persistence或 production readiness。若失敗，依 classifier + response evidence分類 operational failure / root cause；同一 non-compile operational root cause 最多自動重試一次。非 `SOURCE` 分類不得合理化 production source patch；evidence不足時 STOP，不猜。
>
> 若 `getDeviceInfo` 真實成功且 Completion Evidence Guard 成立，本 Stage 可判定完成；完成 queue lifecycle 應移除本 Deferred item，而不是保留 Completed 標記。若後續 `getAirconStat` 確實需要追蹤，另建立新的 read-only Deferred/Pending-validation item，但不得自行執行下一階段。
>
> Hardware/network 授權與 repository mutation / commit / push 授權分開判斷。只有本次 launch 另外明確授權 docs/TASKS mutation 與 Git mutation 時，才可把可驗證結果更新到正式 evidence/docs、依 queue lifecycle 更新 `TASKS.md` 並 commit/push；否則只回報 canonical hardware/network evidence，repository 保持不變。不得提交或輸出 secrets。
