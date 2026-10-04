# Unfinished work queue

## Hot — Correct TimeProvider reconciliation derived documentation

**Goal：** 收斂前一個 TimeProvider machine-readable reconciliation 的 completion drift，只修正因新增兩個 host validator tests與文件行數變化而 stale 的 derived documentation facts；不重開 contract semantics或 implementation。

**Current evidence / authority：**
- Current WF-RAC main: `f8cc5502b61bc7163125e42e59c64ef9b123f8cc`。
- Previous TimeProvider reconciliation core contract / validator / tests / docs semantics已完成；本 corrective 不重新修改其技術結論。
- Current targeted Python host tests = **66**。
- Reconciliation relative to pre-stage baseline produced physical-line deltas:
  - `tests/host/test_validate_contracts.py`: +23
  - `tools/validate_contracts.py`: +5
  - `docs/references/topics/timestamp-time.md`: +1
  - `docs/roadmap.md`: +1
  - total: +30
- Therefore current README derived metrics should be:
  - Firmware: 2776
  - Development / validation tools: 3489
  - Machine-readable specs: 404
  - Technical docs: 700
  - Other tracked project files: 33
  - Total: 7402

**Required corrective：**
- In `docs/roadmap.md`, update the Phase 0 software-only machine-readable foundation test count from 64 to **66** targeted host tests。
- In `README.md`, update only the stale repository physical-line table values:
  - development / validation tools: **3489**
  - technical docs: **700**
  - total: **7402**
- Preserve all other README / roadmap wording and technical claims unless a directly adjacent derived value must change for internal arithmetic consistency。

**Hard exclusions：**
- Do not modify specs, validators, tests, source/components, firmware, architecture, security, references, BACKLOG, evidence, CI/workflows, or runtime。
- Do not change TimeProvider implementation semantics, test logic, Phase 1C scope, or any Cold Candidate。
- Do not recalculate or rewrite unrelated repository metrics beyond the values explicitly identified above unless current canonical files prove the arithmetic assumption wrong。

**Validation / completion gate：**
- Final scoped diff contains only `README.md`, `docs/roadmap.md`, and completion bookkeeping in `TASKS.md`。
- README physical-line table is internally consistent and sums to 7402。
- Roadmap host-test count is 66 and consistent with README / current test suite evidence。
- `git diff --check` PASS。
- No source/build/hardware/network validation required for this docs-only corrective。
- On success, remove this Hot Stage, return `TASKS.md` to minimal `EMPTY`, commit/push, and report final remote HEAD, changed-file list, applicable Playbook revision, and validation result。
