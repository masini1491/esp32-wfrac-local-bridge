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

### Stage 1 — Persist sanitized 2026-09-02 PC WF-RAC local evidence

**推薦模型：** Luna  
**推理強度：** Low  
**推薦理由：** 本 Stage 只把已取得、已判讀的實機證據保存成 canonical provenance dossier，不改 production behavior、protocol implementation 或 machine-readable semantics；資料範圍與去識別化要求已明確。  
**是否值得先用較便宜模型做前置蒐證：** 否；Luna Low 已是最低充分設定。  
**Context 建議：** L0 → L2；先讀最新 `AGENTS.md` / `TASKS.md`、`docs/references/README.md`，再只讀與 evidence dossier placement / credential boundary 直接相關的最低必要 reference/security evidence。  
**Execution mode 建議：** Normal / Agent 1。  
**Admission reason：** 2026-09-02 已取得兩台真實 WF-RAC module 的 PC→LAN local API read/write/coexistence evidence，已跨過一次性除錯門檻；若不保存，後續 C3 hardware bring-up 與 protocol validation 容易重新探索已知事實。  
**前置 evidence（conversation-provided，寫入前必須依本 Stage 只保存最低充分 sanitized facts）：**
- 兩台真實 module 均已以 plain HTTP port 51443 成功完成 `getDeviceInfo` 與 `getAirconStat`，HTTP 200、application `result:0`。
- 兩台回報相同 profile：`firmType=WF-RAC`、wireless firmware `010`、MCU firmware `131`。
- `getDeviceInfo` 回傳的 `airconId` / `macAddress` 與各自實體 module MAC 對應一致；不得把 exact MAC / airconId 寫入 public Git。
- `getAirconStat` 的 Base64 payload 可被既有 parser contract 解出實際 operation/settings/telemetry；本 Stage 不重新定義 parser semantics。
- Google Assistant 成功控制其中一台後，觀察到 `updatedBy=GoogleAssistant` 且 `expires` 刷新，同時 AirconStat 反映實際狀態變化。
- PC local `setAirconStat` 已成功把設定溫度 24.0°C 改為 26.0°C，HTTP 200、`result:0`；成功 write 後 `updatedBy=pc-probe`，且該次 response 的 `expires - timestamp = 60s`。
- PC write lock 到期後，Smart M-Air 官方 client 可再次成功控制同一 module，`updatedBy` / `expires` 由新 writer 刷新，證明此實測 profile 下 local writer 與官方控制可依 lock lifecycle coexist。
- 先前 Windows PowerShell 5.1 直接把 JSON `$body` 傳給 native `curl.exe --data` 所產生的 HTTP 501 / `Not supported this command`，已由 byte-length evidence 與 UTF-8 no-BOM file + `--data-binary @file` 成功結果證實為 quoting/body corruption；舊 501 command-incompatibility interpretation 必須標為 `SUPERSEDED`，不得保留成 current firmware limitation。

**去識別化 / security boundary：**
- 不得提交 Wi-Fi credentials、Operator ID、remote/account UUID、手機 deviceId、exact LAN IP、exact MAC、exact airconId、Authorization/credential material 或其他可識別家庭網路/帳號的值。
- dossier 只使用 `Device A` / `Device B` 或等價匿名標籤；可保存 firmware profile、port/scheme、command/result、狀態轉換、60 秒 observed delta、`GoogleAssistant` / `pc-probe` 這類非 secret writer label，以及 PowerShell/curl root-cause pattern。
- 不得把本次 PC evidence 宣稱為 ESP32-C3/C6 Hardware PASS、production readiness、所有 WF-RAC firmware universal behavior，或 self-managed registration validation。

**目標：** 在 `docs/references/sources/` 建立一份 topic-independent、sanitized、date-stamped local evidence dossier；名稱可採 `local-wfrac-pc-validation-2026-09-02.md` 或依現有 naming convention 的等價名稱。內容至少區分：test scope/profile、CONFIRMED_LOCAL observations、SUPERSEDED diagnostic interpretation、limits/non-generalization、privacy redactions，以及後續 topic reconciliation pointers。必要時更新 `docs/references/README.md` / sources index 的最低充分入口，但不得在本 Stage 修改各 topic 的 authoritative conclusion。

**Validation / completion gate：**
- dossier 可由 current repo reader 明確辨識其 evidence scope 是 Windows PC → real WF-RAC modules，而不是 ESP-IDF target hardware。
- exact credential/private-network identifiers 未出現在 scoped diff。
- `CONFIRMED_LOCAL` / `SUPERSEDED` / limits 表達與最新 Playbook evidence lifecycle一致。
- 不修改 production source、tests、machine-readable contract 或 authoritative topic conclusions。
- 完成前執行最低充分 docs/link/sanitization check；成功後依 queue lifecycle只移除此 Stage，不得自行開始 Stage 2。

### Stage 2 — Reconcile WF-RAC topics against local PC evidence

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 需要把同一批 local evidence 精確映射到 transport、API envelope、identity、time/lock 與 AirconStat/request composition 等既有 authority，並避免把 device-specific evidence過度泛化；屬 bounded multi-document validation-state reconciliation。  
**是否值得先用較便宜模型做前置蒐證：** 否；Stage 1 dossier 完成後已提供最低充分 provenance。  
**Context 建議：** L0 → L3；先讀最新 `AGENTS.md` / `TASKS.md` 與 Stage 1 dossier，再依 `docs/references/README.md` 只讀直接相關 topics：`tls-transport.md`、`api-envelope-discovery.md`、`identity-registration.md`、`timestamp-time.md`；只有 evidence 確實影響時再讀 `request-composition.md` / `airconstat-fixed-state.md`。  
**Execution mode 建議：** Normal / Agent 1。  
**Dependency：** 只有 Stage 1 已完成、已 push，且 Completion Evidence Guard 的 remote read-back 確認 dossier / sanitization 成立後才可啟動。  
**目標：** 依 Stage 1 dossier 對既有 topic 的 current evidence state 做最小必要 reconciliation：
- Transport：對本次兩台 `WF-RAC / wireless 010 / MCU 131` profile，plain HTTP :51443 與 successful local API path 可標 `CONFIRMED_LOCAL`；不得把 per-firmware universal scheme unknown、ESP-IDF runtime mapping、HTTPS/certificate unknown 一併解除。
- API envelope：把「actual HTTP request / real `getAirconStat` response 尚 pending」中已被 PC local evidence解決的部分精確更新；仍保留 discovery、firmware-wide handlers、polling/retry tuning、ESP-IDF runtime 等未驗證範圍。
- Identity：保存 `updatedBy` 與 request `deviceId` 的 local writer evidence、既有 Operator ID 可作 mirror-style write 的觀察；不得保存 exact Operator ID，不得宣稱第二 remote UUID 身分、self-managed registration 或 registration lifecycle已驗證。
- Timestamp / lock：保存本 profile 中成功 write 的 `expires - timestamp = 60s` 與 lock 到期後官方 client可重新寫入的 local evidence；必須明確限制為 observed profile，不得升成所有 firmware universal constant。
- 若 AirconStat/request-composition topic 目前含「write 未實機驗證」等直接被 24→26°C round-trip 推翻的 statement，做最低充分更新；否則不為形式改檔。
- 明確標示先前由 PowerShell/curl body corruption造成的 501 incompatibility interpretation為 `SUPERSEDED`，避免舊 troubleshooting evidence重新污染 current conclusions。

**限制：** 不修改 production source；不新增 protocol behavior；不做新的 network/hardware request；不動 registration/account table；不把 PC evidence提升為 ESP32-C3/C6 Hardware PASS。  
**Validation / completion gate：** 相關 topic 的 `CURRENT` / `SUPERSEDED` / `HARDWARE_TEST_PENDING` 邊界彼此一致，且所有 local claim 都能回指 Stage 1 dossier；docs/link/contract consistency 的最低充分 validation PASS。成功後依 queue lifecycle只移除此 Stage，不得自行開始 Stage 3。

### Stage 3 — Reconcile machine-readable evidence state and C3 probe pre-evidence

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 需要先判斷 machine-readable contract 是否真的承載 local evidence state，再只更新可精確表示的部分；同時要保留 PC evidence 與 ESP32-C3/C6 hardware validation 的 target boundary。  
**是否值得先用較便宜模型做前置蒐證：** 可；若只檢查 `specs/wf-rac-contracts.json` 是否存在對應 evidence-state欄位，可先由 Luna Low 唯讀確認後 STOP；正式 mutation/reconciliation仍使用 Terra Medium。  
**Context 建議：** L0 → L3；先讀最新 `AGENTS.md` / `TASKS.md`、Stage 1 dossier與 Stage 2 已更新 topics，再只讀 `specs/wf-rac-contracts.json` 直接相關 sections、validator/tests，以及既有 `First real WF-RAC read-only probe / hardware validation` Stage。  
**Execution mode 建議：** Normal / Agent 1。  
**Dependency：** 只有 Stage 2 已完成、已 push，且 Completion Evidence Guard remote read-back確認 topic reconciliation成立後才可啟動。  
**目標：**
- 先判斷 machine-readable contract 是否有適合、且語意足以表達「specific local PC evidence confirmed while ESP-IDF target hardware remains pending」的欄位；只有能精確表達時才修改 contract / validator / targeted tests。若 schema 只代表 universal protocol truth，不得為本地兩台實測而把 universal unknown 改成 confirmed；此時保持 machine contract不變並記錄 N/A rationale。
- 檢查既有 `First real WF-RAC read-only probe / hardware validation` Stage 的 pre-evidence 是否需要最小更新：可引用已知-good PC ground truth（target modules on plain HTTP :51443、`getDeviceInfo` / `getAirconStat` known-good、profile已知），但該 Stage 必須繼續保持 ESP32-C3/C6 Hardware/Network pending，不得因 PC結果完成或刪除。
- 若 C3 probe 的 explicit scheme / expected response inputs 因 Stage 1/2 evidence 已明確，可收斂其探索範圍，讓下一次 hardware Stage只驗證 ESP-IDF/C3 transport implementation是否重現 known-good device behavior。

**限制：** 不執行 hardware/network test；不修改 production runtime；不把 local profile generalize成 universal firmware contract；不提前完成 C3 hardware Stage。  
**Validation / completion gate：** machine-readable contract 若有修改，validator/targeted tests與相關 docs一致且 host validation PASS；若判定不應修改，必須以 canonical schema語意證據說明 N/A。既有 C3 hardware Stage仍存在且 target boundary正確。成功後依 queue lifecycle移除此 Stage。

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
> 將結果明確分成 Software/Compile/Static-Test/Bench/Hardware/Network evidence；API/transport call accepted 不等於 WF-RAC application success。Hardware/Network evidence只對本次實際 target/board/device/WF-RAC/network environment與已執行的 read-only `getDeviceInfo` path成立，不得外推到另一 board/FQBN、write path、polling、Matter、persistence或 production readiness。若失敗，依 classifier + response evidence分類 operational failure / root cause；同一 non-compile operational root cause最多自動重試一次。非 `SOURCE` 分類不得合理化 production source patch；evidence不足時 STOP，不猜。
>
> 若 `getDeviceInfo` 真實成功且 Completion Evidence Guard成立，本 Stage可判定完成；完成 queue lifecycle應移除本 Deferred item，而不是保留 Completed標記。若後續 `getAirconStat` 確實需要追蹤，另建立新的 read-only Deferred/Pending-validation item，但不得自行執行下一階段。
>
> Hardware/network授權與 repository mutation / commit / push授權分開判斷。只有本次 launch另外明確授權 docs/TASKS mutation與 Git mutation時，才可把可驗證結果更新到正式 evidence/docs、依 queue lifecycle更新 `TASKS.md`並 commit/push；否則只回報 canonical hardware/network evidence，repository保持不變。不得提交或輸出 secrets。
