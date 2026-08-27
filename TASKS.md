# Unfinished work queue

此檔只追蹤尚未完成、Blocked、Deferred 或待驗證工作。完成項目應直接移除；Git history 才是完成紀錄。不得因看到本檔而自行執行未明確授權的工作。

## Codex queue / launch rules

- 每次只執行使用者當次明確授權的 Stage；`TASKS.md` 本身不授權開始其他項目。
- 共通的 repository identity、workspace / permission / credential capability、remote sync、Git safety、failure taxonomy / retry、validation / evidence、Completion Evidence Guard、Progressive Repository Reading、Model / Reasoning / Context / Agent / cost control、long-running operation 與 reporting timestamp，均以最新 `AGENTS.md` routing 到 `masini1491/ai-development-playbook` 的規則為 authority；本檔不複製完整 common policy，避免 drift。
- `TASKS.md` 只收錄需要未來追蹤或再次執行的 TODO / Blocked / Deferred / Pending-validation 工作。一次性、已知位置、已知修改內容或 root cause、低風險且完成後無追蹤價值的小型 maintenance，不必為形式建立 queue item。
- 本檔的推薦模型、推理強度、Context、Execution mode 只是給使用者的 launch 建議；Codex 不得自行升降模型、推理強度、Context、Fast / Ultra 或 Multi-Agent。若 evidence 顯示目前設定不足，STOP、保留 evidence，由使用者重新 launch。
- 執行 Stage 前先安全同步最新 `origin/main`，完成 Repository Identity / workspace capability / Git state / permission gates；只允許 clean、expected `main`、無 local ahead/diverged 且可 fast-forward-only 的同步。同步後重新讀最新 `AGENTS.md` / `TASKS.md`；若 Stage 已消失或 scope 已改變，STOP，不得依舊 Prompt 或記憶執行。
- Repository reading 從目前 Stage 的最低充分 evidence 開始，依 routed Progressive Repository Reading 逐級擴張；禁止為熟悉專案預設 repo-wide 掃描。
- 不得因執行途中發現其他問題就順便跨 Stage、architecture、runtime、hardware、deployment 或其他 mutation boundary；out-of-scope work 留待新 task / Stage 或 STOP 回報。
- Stage 宣稱完成、更新 validation state、移除 queue item、commit / push 或進入下一 Stage 前，必須依 Completion Evidence Guard，以最低充分 canonical repository evidence 交叉確認 current HEAD / scoped diff / required validation / `TASKS.md` state。自然語言 completion report 不得覆蓋 repository evidence。
- 成功且證據完整後刪除已完成 Stage，不保留 Completed 區段；仍有 Deferred / Blocked / TODO / Pending-validation 時保留本檔，完全無未完成工作時刪除 `TASKS.md`。

### Codex 短指令啟動格式

使用者可在 Codex UI 手動選好該 Stage 指定的模型與推理強度後，只貼短指令：

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP；必要 operation 若缺權限，依最新 `AGENTS.md` routing 的 common playbook 要求最小必要 capability，不得自行繞過）。同步後讀根目錄 `AGENTS.md` 與 `TASKS.md`，依 `AGENTS.md` routing 只讀本 Stage 所需的最低必要 Playbook / project evidence，完整執行 `TASKS.md` 的「<Stage 名稱>」，只執行該 Stage。完成前依 Completion Evidence Guard 交叉確認 canonical repository / validation / queue state；成功後依 queue lifecycle 更新 `TASKS.md`、commit / push（僅限本次 launch 已授權者）。

## Deferred

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
