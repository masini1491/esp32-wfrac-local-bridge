# Unfinished work queue

## Hot — Clarify project-native shared-Playbook activation

**Goal：** 修正 `AGENTS.md` 中 shared Playbook activation wording，明確落實 current `masini1491/ai-development-playbook` 的「adoption ≠ unconditional activation」與 project-native activation gate；不重做已完成的 ownership normalization。

**Required correction：**
- 明確寫出 bounded ordinary WF-RAC technical query 先走 project-native technical owners；在 evidence 已充分時停止，不因已採用 Playbook 就自動 resolve／載入 shared `CHAT_INIT.md`。
- 只有 governance、repository mutation、Git/repository operation、validation/completion、AI workflow／handoff／其他 current project routing 判定需要 shared governance 的 action，才 resolve floating baseline `main` 至 exact revision，再進 shared `CHAT_INIT.md` 與最低充分 owner。
- 保留 `Project AI mode: ChatGPT+Codex`、existing technical-owner map、ChatGPT write allowlist、Phase 1C boundary、security/toolchain/portable-library project-specific exceptions。

**Hard exclusions：**
- 只修改 `AGENTS.md` 與 completion bookkeeping 所需的 `TASKS.md`。
- 不修改 `BACKLOG.md`、README、docs、specs、source、tests、tools、workflow、evidence 或 runtime。
- 不改 technical truth、current phase、AI mode、write authority、Candidate commitment或任何 validation state。

**Completion gate：**
- `AGENTS.md` 明確區分 project-native ordinary technical route 與 conditional shared-Playbook activation route，且不形成第二份 shared methodology。
- 原本 normalization 已驗收通過的 project-specific authority／exceptions全部保留。
- scoped diff僅 `AGENTS.md`、`TASKS.md`；`git diff --check` PASS；remote canonical read-back PASS。
- 成功後移除本 Hot Stage，將 `TASKS.md` 收斂回最低充分 `EMPTY`，commit／push並回報 final remote HEAD。
