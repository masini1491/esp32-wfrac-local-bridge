# Unfinished work queue

## Hot — Slim shared routing duplication in project governance

**Goal：** 依 current `masini1491/ai-development-playbook` ownership／routing architecture，移除 `AGENTS.md` 中已由 shared Playbook canonical routing 擁有、且容易隨 owner relocation 漂移的 project-local shared owner catalogue，並消除 `Common playbook routing` 與 structured adoption declaration 的重複 baseline／AI-mode ownership；保留 project-native conditional activation、technical authority 與所有 stricter project exceptions。

**Current authority / decisions：**
- Playbook baseline remains `main`；Project AI mode remains `ChatGPT+Codex`。
- Current structured sections `AI Development Playbook baseline`、`Authority boundary`、`Project-specific minimum contract` remain canonical project adoption declarations。
- Ordinary bounded WF-RAC technical work remains project-native；shared governance only activates when current project routing requires it。
- Activation-independent substantive user-facing reporting continues through declared-baseline `REPORTING.md` without activating other shared owners。
- Current Phase 1C technical truth、technical-owner map、ChatGPT coordination write allowlist、security/toolchain/portable-library restrictions remain unchanged。

**Required change：**
- Refactor the current `Common playbook routing` content into a thin project-native shared-governance activation rule only; do not repeat the baseline or Project AI mode already owned by the structured adoption declaration。
- Remove the project-local enumerated shared owner catalogue (`AI_CONTEXT.md`, `REPOSITORY_EXECUTION.md`, `DEBUG_VALIDATION.md`, `RESEARCH_ARCHITECTURE.md`, `EMBEDDED_PROJECTS.md`, `CHATGPT_WORKFLOW.md`, `CODEX_EXECUTION.md`, `TOOLCHAIN.md`, etc.) so future shared owner movement does not require WF-RAC edits。
- Retain only the minimum stable shared entry pointers needed by this project:
  - shared governance activation → declared baseline exact revision → `CHAT_INIT.md`
  - substantive user-facing reporting → declared baseline `REPORTING.md`
- Do not create a second router, index, checker, or mirror of Playbook ownership。
- Preserve all project-native owner pointers and restrictions already required by the structured adoption / minimum contract。
- Re-run the current Adoption Doctor semantics after the edit and preserve Doctor-clean status; do not satisfy parser checks with duplicated or misleading wording。

**Hard exclusions：**
- Only modify `AGENTS.md` and completion bookkeeping in `TASKS.md`。
- Do not modify `BACKLOG.md`, `evidence/**`, README, docs, specs, components/source, firmware, tests, tools, workflow/CI, or runtime。
- Do not change Phase 1C scope, technical truth, C6/C3 targets, security/credential policy, portable-library boundary, Cold Candidates, evidence taxonomy, write authority, or Project AI mode。

**Completion gate：**
- Final scoped diff contains only `AGENTS.md` and `TASKS.md`。
- `AGENTS.md` has one structured baseline/mode declaration and one thin project-native activation rule; no duplicate baseline/mode ownership remains outside the structured adoption section。
- No project-local full shared-owner catalogue remains; shared routing is delegated to exact-revision `CHAT_INIT.md`, while `REPORTING.md` remains the explicit activation-independent reporting pointer。
- Current Adoption Doctor remains Doctor-clean (0 FAIL / 0 WARN), and bounded routing/read-back confirms ordinary project hot path does not gain extra shared loading。
- Existing project-native technical authority / restrictions remain materially unchanged。
- `git diff --check` PASS; no source/build/hardware validation is required for this docs-only Stage。
- On success, remove this Hot Stage, return `TASKS.md` to minimal `EMPTY`, commit/push, and report final remote HEAD, changed-file list, Doctor result, and applicable Playbook revision。
