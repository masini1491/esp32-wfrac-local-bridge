# Unfinished work queue

此檔只追蹤尚未完成、Blocked、Deferred 或待驗證工作。完成項目應直接移除；Git history 才是完成紀錄。不得因看到本檔而自行執行未明確授權的工作。

## Codex 共通執行規則

- 每次只執行使用者當次明確授權的 stage；不得因本佇列存在就自行處理其他項目。
- **TASKS.md 使用判準：** `TASKS.md` 是「未完成、需要追蹤、需要未來再次執行」的 queue，不是所有微小修改都必須先建立 task。若只是一次性、已知位置、已知修改內容、風險低，而且完成後沒有後續追蹤價值的小型 maintenance，不必特地寫入 `TASKS.md`；可由 ChatGPT 直接產生一次性的短 Codex Prompt。典型例子包括：單行或少量文件 wording 修正、已知位置的 typo / 過期術語修正、單純排序/格式/標題/註解整理、不影響 project behavior / architecture / protocol / security / hardware / validation state 的小型 documentation maintenance，以及已知 root cause、scope 極小且完成後不需保留為 unfinished queue 的工作。這類 direct maintenance 預設建議 **Luna / Low / Context Level 0→1 / Focused patch / Agent 1**；Codex 仍必須遵守 `AGENTS.md` 的 remote-sync、scope、Git、validation、commit/push 規則。
- **應寫入 TASKS.md 的情況：** 需要後續追蹤；目前 Blocked / Deferred / Pending-validation；工作分成多個 Stage；有 dependency / trigger；root cause 尚未完全確認；可能需要之後接續 implementation；會實質影響 project behavior、architecture、protocol、security、hardware、persistence、runtime state 或重要 validation state；或若現在不記錄，之後容易遺漏。
- **模型／推理／執行設定控制權：** `TASKS.md` 內的推薦模型、推理強度、Context、Execution mode 是 ChatGPT 規劃給使用者的建議，不是 Codex 自行切換或升級設定的授權。使用者在 Codex UI 手動選擇模型與推理強度；Codex 不得自行 `Luna → Terra → Sol`，也不得自行 `Low → Medium → High`，不得把 escalation condition 解讀為可自行擴大 Context、切換 Fast/Ultra 或啟用 Multi-Agent。若目前 stage 達到 escalation condition，Codex 必須 STOP、保留既有 evidence 並回報，由使用者決定是否更換模型／推理強度／Context／Execution mode 後重新 launch。`TOOLCHAIN` / `ENVIRONMENT` / `INFRASTRUCTURE` / `SERVICE` / `HARDWARE_REQUIRED` failure 本身不得作為升模型或提高推理強度的理由。
- **Remote-sync bootstrap：** Codex 本機 repository 可能落後 GitHub。每次 task 開始，在依賴 local `AGENTS.md` / `TASKS.md` 內容前，先確認 repository identity、`git status --short`、branch、HEAD，然後 `git fetch origin`。若目前為預期 `main`、working tree clean，且 local `main` 可由 `origin/main` fast-forward，使用 fast-forward-only 方式同步（例如 `git pull --ff-only origin main`）；若已與 `origin/main` 相同則直接繼續。若 dirty、unexpected branch、local ahead/diverged、無法 fast-forward，或 merge/rebase/cherry-pick 未完成，STOP 並回報，不得覆蓋、reset、stash/丟棄未知變更或自行 merge/rebase。**只有完成這個同步判斷後，才以最新 local `AGENTS.md` / `TASKS.md` 為 task source。**
- **Permission-Gated Operation：** 若目前 stage 的必要 Git/build/test/toolchain/file operation 遭 sandbox／filesystem／execution permission denial，遵守 `AGENTS.md` 的 Permission-Gated Operation 規則；可向使用者要求完成原必要操作所需的最小權限。單純 permission gate 尚不算 operational failure，也不消耗 retry cap；不得自行繞過 permission、安全限制、Git safety 或擴大 stage scope。
- **Progressive Repository Reading：** 同步後讀 `AGENTS.md`、本 `TASKS.md` 與該 stage 直接相關 evidence/files，從最小足夠範圍開始，只有 evidence 不足才逐級擴張：**L0** = Git preflight + 最新 `AGENTS.md` / `TASKS.md` / current evidence；**L1** = 直接 symbol / target / test；**L2** = caller / callee / owner / direct dependency；**L3** = 完整 relevant file；**L4** = relevant module / directory；**L5** = repo-wide search / read。禁止預設 repo-wide 掃描或跳級讀取。若同步後根目錄沒有 `TASKS.md`，不得憑舊記憶執行已移除的 stage；依使用者當次明確指令處理或 STOP 詢問。
- Git safety：不得 reset --hard、force push、丟棄或改寫未知變更；不得為了讓工作樹變乾淨而自行 stash/delete user work。
- Debug / patch 順序：Evidence → Root Cause → Focused Patch → Targeted Validation。Root cause 只能標為 `CONFIRMED ROOT CAUSE`、`HIGH-CONFIDENCE LIKELY ROOT CAUSE`、`INSUFFICIENT OBSERVABILITY`；只有前兩者可直接 patch，第三種先加最小觀測或 STOP。
- 同一 root cause 自動 retry 最多 1 次；第二次失敗時分類 `SOURCE` / `TOOLCHAIN` / `ENVIRONMENT` / `INFRASTRUCTURE` / `SERVICE` / `HARDWARE_REQUIRED`，不要無限重跑。
- Validation 由小到大：static/targeted verifier → targeted test → related build → 必要 matrix → full regression。不要為小 patch 預設跑最大驗證。
- B/B2/C/D 預設 **Context：Default**、**Execution mode：Normal / single-agent**；不得使用 1M context、Fast、Ultra 或 Multi-Agent，除非該 stage 明確記載升級條件且有新 evidence 支持；即使升級條件成立，也必須依前述控制權規則 STOP，由使用者決定並重新 launch，不得由 Codex 自行切換。
- 若目前使用者所選模型／推理強度在已指定 root cause / patch direction 下仍無法安全完成，Codex 必須保留 evidence、STOP 並回報；不得自行升級模型或推理強度，也不得無證據建議 Sol。
- 成功驗證後必須同步更新本檔：**刪除整個已完成 stage 的 prompt/metadata 與對應 inventory 項目**，不要留下 Completed 記錄。若尚有 Deferred/Blocked/TODO 則保留本檔；完全無未完成工作時刪除 `TASKS.md`。
- Commit/push 前 `git diff --check`；只提交本 stage scope。禁止 force push/history rewrite。
- Codex final report 保持精簡，只列：`Root cause / research gap`、`Modified files`、`Validation`、`Pending / TASKS.md state`；不要重述大量背景。

### Codex 短指令啟動格式

使用者可在 Codex UI 手動選好本 stage 指定的模型與推理強度後，只貼短指令：

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP；若必要操作因權限不足被拒絕，依共通規則向我要求最小必要權限，不得自行繞過）。同步後讀根目錄 `AGENTS.md` 與 `TASKS.md`，完整執行 `TASKS.md` 的「<Stage 名稱>」，只執行該 stage。成功驗證後依 `TASKS.md` 規則更新 queue、commit 並 push。

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
