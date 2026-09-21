# Unfinished work queue

此檔是本 repository 的 current Hot coordination surface，只追蹤目前 executable／critical-path、已正式 admission 的工作；完成項目直接移除，Git history 才是完成紀錄。不得因看到本檔而自行執行未經使用者當次明確授權的 Stage。

## Hot — Project governance ownership normalization

**Goal：** 依 current `masini1491/ai-development-playbook` ownership architecture，將跨專案共通方法論上移到 shared canonical owners，WF-RAC 本地只保留 project mapping、project-native authority／stricter exceptions 與必要 routing；這是 docs-only governance normalization，不改變產品、runtime、protocol、security 或 validation truth。

**Current decisions / authority：**
- Playbook baseline：`main`。
- Project AI mode：`ChatGPT+Codex`；將此既有使用者決策持久化到 project governance。
- Current technical phase remains `Phase 1C — WF-RAC Transport Abstraction`；phase/current capability 不等於 execution authority。
- Existing technical owners remain authoritative：`docs/architecture.md`、`docs/security.md`、`docs/roadmap.md`、`docs/references/README.md`／`docs/references/**`、`specs/wf-rac-contracts.json` 及 current source/tests/evidence。

**Required normalization：**
- `AGENTS.md`：保留 Playbook adoption／baseline／AI mode、project-native activation/mapping、WF-RAC-specific stricter restrictions、coordination write allowlist 與最低充分 technical-owner pointers；移除或收斂 shared Playbook 已擁有的 generic bootstrap、owner catalogue、Hot/Cold/Evidence lifecycle、generic permission/capability、model/delegation、debug/retry/validation/reporting 等 methodology duplication。
- 釐清 current phase／implemented capability 與 Task/Stage execution authority；不得再用 Phase 1C 的「允許項目」文字形成獨立 execution grant。必要 current technical state應由既有 technical owners承接或由 AGENTS 只保留最低 survival wording。
- `TASKS.md`：維持真正 Hot surface；移除 project-local generic Codex launch／safe-sync／Completion Evidence Guard／Prompt template 副本。Stage完成後收斂成最低充分 `EMPTY` Hot state與 Cold／Evidence pointers。
- `BACKLOG.md`：只瘦身通用 Cold lifecycle header；三個現有 `CANDIDATE` 的 identity、goal、trigger、exclusions、completion gate與 commitment level保持不變。
- `/evidence/inbox/*.md` 既有 opt-in與 sanitized staging role保持有效；不建立第二份 validation/architecture truth。
- 先做 derived dependency／routing integrity check；只在 correctness 必須時更新上述三個治理／coordination files，不新增新 owner、新 router 或新 information surface。

**Hard exclusions：**
- 不修改 `README.md`、`docs/**`、`specs/**`、`components/**`、`firmware/**`、`tests/**`、`tools/**`、workflow／CI 或 `evidence/inbox/*`。
- 不改 Phase 1C technical truth、C6/C3 target、layering、protocol、TLS/security、credential policy、portable-library boundary、roadmap state、existing evidence taxonomy 或任何 runtime behavior。
- 不改三個 BACKLOG Candidate 的 recommendation／commitment authority。
- 不因 normalization 擴張 ChatGPT write authority或 Codex implementation authority。

**Completion gate：**
- Final scoped diff 只應包含 `AGENTS.md`、`TASKS.md`，以及必要時僅 header-normalization 的 `BACKLOG.md`。
- Remote canonical governance 明確存在 single Playbook baseline `main` + `Project AI mode: ChatGPT+Codex`。
- Shared methodology duplication materially reduced；project-native hardware/protocol/security/technical authority沒有被刪弱或搬成 shared truth。
- Project-native activation/routing能讓普通 project technical query 不因採用 Playbook而無條件 full-load shared governance；需要 governance/mutation/Git/validation/handoff 時仍能依 current shared `CHAT_INIT.md` closure最低充分 owner。
- `TASKS.md` 完成後為最低充分 `EMPTY`；`BACKLOG.md` 三個 Candidates語意與 commitment不變；`evidence/inbox` contract仍有效。
- 做最低充分 routing/read-back與 `git diff --check`；不為此 docs-only Stage跑 source/build/hardware validation。
- 成功後 commit／push，回報 final remote HEAD與 changed-file list。

## Codex queue / launch rules

- 每次只執行使用者當次明確授權的 Stage；`TASKS.md` 本身不授權開始其他項目。
- 共通 repository identity、permission、remote sync、Git safety、AI Context、validation、evidence、Completion Evidence Guard 與 coordination lifecycle，以最新 `AGENTS.md` routing 到 `masini1491/ai-development-playbook` 的 canonical authority 為準；本檔不複製完整 common policy。
- 執行 Stage 前先安全同步最新 `origin/main`，重新讀最新 `AGENTS.md`、`TASKS.md` 與本 Stage 直接相關的最低必要 authority；若 Stage 已消失、scope materially 改變或 repository identity／Git state 不符，STOP。
- Repository reading 採最低充分 retrieval；禁止為熟悉專案預設 repo-wide 或 full-playbook 掃描。
- 不得因執行途中發現其他改善就自行跨 Stage、擴大治理 surface，或順手修改 source/runtime/hardware。
- Stage 宣稱完成、更新 coordination state、commit／push 或移除本 Stage 前，必須依 Completion Evidence Guard 交叉確認 current HEAD、scoped diff、必要 validation 與 coordination state。

### Codex 短指令啟動格式

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP）。同步後讀根目錄 `AGENTS.md`、`TASKS.md`，並依 `AGENTS.md`／最新版 common playbook 只讀本 Stage 所需的最低必要 authority，完整執行 `TASKS.md` 指定 Stage；完成前依 Completion Evidence Guard 交叉確認 canonical repository、coordination state 與 scoped validation；成功後更新 coordination bookkeeping、commit／push（僅限本次 launch 已授權者）。
