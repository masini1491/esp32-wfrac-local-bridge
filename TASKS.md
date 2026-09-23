# Unfinished work queue

## Hot — Refresh Playbook adoption declaration and reporting routing

**Goal：** 依 current `masini1491/ai-development-playbook` adopter contract，讓 WF-RAC 的 shared-Playbook adoption declaration 可被最新版 Adoption Doctor穩定解析，接上 activation-independent shared `REPORTING.md`，並移除容易隨 shared ownership 漂移的 project-local owner catalogue；維持既有 project-native conditional activation 與 technical authority。

**Current decisions / authority：**
- Playbook baseline remains floating `main`。
- Project AI mode remains `ChatGPT+Codex`。
- Existing conditional activation remains：bounded ordinary WF-RAC technical query 先走 project-native technical owners；只有 governance／repository mutation／Git／validation-completion／AI workflow-handoff等需要 shared governance時，才 resolve baseline至 exact revision並進 shared `CHAT_INIT.md`。
- Existing project-native technical owners、Phase 1C boundary、ChatGPT coordination write allowlist、security/toolchain/portable-library restrictions remain authoritative。

**Required refresh：**
- 在 `AGENTS.md` 建立單一、active、可機讀的 preferred adoption declaration layer，至少明確包含：
  - `## AI Development Playbook baseline`
  - `Playbook baseline: main`
  - `Project AI mode: ChatGPT+Codex`
  - thin shared reporting pointer：declared Playbook baseline → `REPORTING.md`，對 substantive user-facing engineering replies activation-independent。
- 建立單一 `## Authority boundary`，把既有 project-specific authority priority 與 adoption/mode 不擴張 Task／write／execution／permission／credential／validation authority 的語義放入該 owner；不得複製 shared actor policy全文。
- 建立單一 `## Project-specific minimum contract`，以 pointers/summary 宣告：
  - Canonical technical source(s)
  - Current coordination surface = `TASKS.md`
  - Required validation
  - Project-specific exceptions or restrictions
  欄位只承載最低充分 declaration，不把 technical truth或 shared methodology搬進來。
- 保留 conditional-activation semantics；shared `REPORTING.md` 是窄化的 adoption-level exception，不因此 activate `CHAT_INIT.md` 或其他 shared owners。
- 收斂目前 project-local shared owner catalogue：除 project-native activation所需的 `CHAT_INIT.md`、activation-independent `REPORTING.md` 與必要 project-specific pointers外，不維護一份會隨 Playbook ownership移動而 drift 的完整 shared owner清單。需要 shared governance時由 exact-revision `CHAT_INIT.md` 做 routing。
- 修正現有 stale wording，例如不得再把共通 user-facing reporting歸為 `CODEX_EXECUTION.md` ownership；Codex-specific execution/reporting delta仍由 shared Playbook current routing決定。
- 以最新版 Adoption Doctor contract做最低充分 deterministic/read-back validation；目標為沒有由本次可修正 adoption declaration造成的 FAIL/WARN（Doctor-clean）。若 Doctor因 project不適用的合法條件仍產生 INFO，保留其 scope，不為清零數字扭曲治理。

**Hard exclusions：**
- 只修改 `AGENTS.md` 與 completion bookkeeping所需的 `TASKS.md`。
- 不修改 `BACKLOG.md`、`evidence/**`、README、docs、specs、components/source、firmware、tests、tools、workflow／CI或runtime。
- 不改 Phase 1C technical truth、C6/C3 targets、protocol/TLS/security truth、credential policy、portable-library boundary、three Cold Candidates或既有 evidence taxonomy。
- 不把 Playbook minimal example整份複製進專案，也不新增 project-local Doctor/checker/router/schema。

**Completion gate：**
- scoped diff僅 `AGENTS.md`、`TASKS.md`。
- current `AGENTS.md` 對最新版 Adoption Doctor 的 applicable adoption checks達到 Doctor-clean（0 FAIL / 0 WARN），或若有無法合法消除的 warning，明確證明其不適用／外部 blocker，不得用 wording hack騙過 parser。
- single active baseline、single AI mode、single authority boundary、single project-specific minimum contract成立；conditional activation與 shared reporting exception都能由 expected entry surface直接發現。
- project-native technical authority／restrictions沒有被 shared declaration削弱或重複搬移；ordinary project hot path不因 refresh增加不必要 shared activation。
- routing/read-back與 `git diff --check` PASS；此 docs-only Stage不跑 source/build/hardware validation。
- 成功後移除本 Hot Stage，將 `TASKS.md` 收斂回最低充分 `EMPTY`，commit／push並回報 final remote HEAD、changed-file list、Doctor結果與 applicable Playbook revision。
