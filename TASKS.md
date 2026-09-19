# Unfinished work queue

此檔是本 repository 的 current Hot coordination surface，只追蹤目前 executable／critical-path、已正式 admission 的工作；完成項目直接移除，Git history 才是完成紀錄。不得因看到本檔而自行執行未經使用者當次明確授權的 Stage。

目前沒有已 admission 且可執行的 Hot task：`EMPTY`。Cold work 請見 [`BACKLOG.md`](BACKLOG.md)；sanitized evidence staging 請見 [`evidence/inbox/`](evidence/inbox/)。兩者都不具 execution authority，只有依 current authority/evidence reconciliation、promotion 到 Hot 並取得使用者明確 launch 後才可執行。

## Codex queue / launch rules

- 每次只執行使用者當次明確授權的 Stage；`TASKS.md` 本身不授權開始其他項目。
- 共通 repository identity、permission、remote sync、Git safety、AI Context、validation、evidence、Completion Evidence Guard 與 coordination lifecycle，以最新 `AGENTS.md` routing 到 `masini1491/ai-development-playbook` 的 canonical authority 為準；本檔不複製完整 common policy。
- 執行 Stage 前先安全同步最新 `origin/main`，重新讀最新 `AGENTS.md`、`TASKS.md` 與本 Stage 直接相關的最低必要 authority；若 Stage 已消失、scope materially 改變或 repository identity／Git state 不符，STOP。
- Repository reading 採最低充分 retrieval；禁止為熟悉專案預設 repo-wide 或 full-playbook 掃描。
- 不得因執行途中發現其他改善就自行跨 Stage、擴大治理 surface，或順手修改 source/runtime/hardware。
- Stage 宣稱完成、更新 coordination state、commit／push 或移除本 Stage 前，必須依 Completion Evidence Guard 交叉確認 current HEAD、scoped diff、必要 validation 與 coordination state。

### Codex 短指令啟動格式

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP）。同步後讀根目錄 `AGENTS.md`、`TASKS.md`，並依 `AGENTS.md`／最新版 common playbook 只讀本 Stage 所需的最低必要 authority，完整執行 `TASKS.md` 指定 Stage；完成前依 Completion Evidence Guard 交叉確認 canonical repository、coordination state 與 scoped validation；成功後更新 coordination bookkeeping、commit／push（僅限本次 launch 已授權者）。
