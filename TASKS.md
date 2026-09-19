# Unfinished work queue

此檔是本 repository 的 current Hot coordination surface，只追蹤目前 executable／critical-path、已正式 admission 的工作；完成項目直接移除，Git history 才是完成紀錄。不得因看到本檔而自行執行未經使用者當次明確授權的 Stage。

## Hot — Adopt sanitized evidence staging surface

- Goal：保留既有 `/TASKS.md`（Hot）與 `/BACKLOG.md`（Cold）有效治理，並新增 ChatGPT 可維護的 sanitized evidence staging surface：`/evidence/inbox/*.md`。
- Actor：本 Stage 涉及 `AGENTS.md`／governance mutation，依目前 `ChatGPT+Codex` collaboration 由 Codex 執行 implementation；ChatGPT 負責 planning 與 completion reconciliation。
- Required governance semantics：
  - `/evidence/inbox/*.md` 僅保存 repo-safe、sanitized observation／現場或外部 evidence 與 provenance staging；預設不載入，無 execution authority，也不是 architecture／protocol／security／validation canonical truth。
  - 不得先提交 secret/private raw material 再刪除；credential、Operator ID、Wi-Fi secret 等仍遵守既有 security boundary。
  - 已 canonicalized 的 `docs/references/sources/*` 不遷移、不複製回 inbox；inbox 只作 staging，reconciliation 後應收斂成 canonical pointer／必要 delta 或移除。
  - 未列入 allowlist 的其他 path 維持 read-only；不得藉本 Stage 擴大 ChatGPT 對 source、tests、README、其他 docs 或 governance 的 direct-write authority。
- Implementation scope：依最新版 common Playbook 的 Multi-Surface adoption procedure，最低充分更新 project governance／routing，建立 evidence staging path 與必要的非 allowlisted surface contract；先檢查 derived dependencies，只有 correctness 必需時才同步相關 classification。不要做 AGENTS slimming、README 整理、source/runtime/hardware 修改或其他無關重構。
- Completion gate：remote canonical state 明確列出 `/TASKS.md`、`/BACKLOG.md`、`/evidence/inbox/*.md` 的責任與 ChatGPT write boundary；evidence semantic／sanitization／default-load／no-execution-authority 清楚；existing canonical evidence 不被重複搬移；必要 targeted validation/read-back PASS。完成後移除此 Hot Stage，將 `TASKS.md` 收斂回 `EMPTY`，commit／push。

## Codex queue / launch rules

- 每次只執行使用者當次明確授權的 Stage；`TASKS.md` 本身不授權開始其他項目。
- 共通 repository identity、permission、remote sync、Git safety、AI Context、validation、evidence、Completion Evidence Guard 與 coordination lifecycle，以最新 `AGENTS.md` routing 到 `masini1491/ai-development-playbook` 的 canonical authority 為準；本檔不複製完整 common policy。
- 執行 Stage 前先安全同步最新 `origin/main`，重新讀最新 `AGENTS.md`、`TASKS.md` 與本 Stage 直接相關的最低必要 authority；若 Stage 已消失、scope materially 改變或 repository identity／Git state 不符，STOP。
- Repository reading 採最低充分 retrieval；禁止為熟悉專案預設 repo-wide 或 full-playbook 掃描。
- 不得因執行途中發現其他改善就自行跨 Stage、擴大治理 surface，或順手修改 source/runtime/hardware。
- Stage 宣稱完成、更新 coordination state、commit／push 或移除本 Stage 前，必須依 Completion Evidence Guard 交叉確認 current HEAD、scoped diff、必要 validation 與 coordination state。

### Codex 短指令啟動格式

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP）。同步後讀根目錄 `AGENTS.md`、`TASKS.md`，並依 `AGENTS.md`／最新版 common playbook 只讀本 Stage 所需的最低必要 authority，完整執行 `TASKS.md` 指定 Stage；完成前依 Completion Evidence Guard 交叉確認 canonical repository、coordination state 與 scoped validation；成功後更新 coordination bookkeeping、commit／push（僅限本次 launch 已授權者）。
