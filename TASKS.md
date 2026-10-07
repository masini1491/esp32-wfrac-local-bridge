# Unfinished work queue

## Hot — Repair roadmap operation-data link introduced by 0xB1 closure

**Goal：** 修正 `docs/roadmap.md` 中由 `0xB1` Stage 新引入的 broken relative link；只把不存在的 `references/operation-data.md` 改回現存的 `references/topics/operation-data.md`，不改其他文字、語意、實作或 scope。

**User-authorized scope（2026-10-07）：**
- 使用者已明確要求執行前述 minimal cleanup。
- 本 Stage 僅授權 `docs/roadmap.md` 單檔最小修正與必要驗證。
- 不授權 source/tests/spec 變更、不授權 residual semantic tightening、vane/3D Auto/advanced fixed-state、hardware/network 或其他 Cold work。

**Evidence / cause：**
- Baseline `8acf5d499c707d4b61d7b18e5b5b12111871b9ab` 的 roadmap 使用正確路徑 `references/topics/operation-data.md`。
- `0xB1` implementation `2711bd962923a3ee65764fe93bc5d7dc4b5a0054` 把該 link 改成 `references/operation-data.md`。
- Final closure `b7f95cd10b2b22dbf3242d2927924b20f76d8421` 中 `docs/references/operation-data.md` 不存在（404）；`docs/references/topics/operation-data.md` 存在。
- 此為 documentation-link defect，不影響已接受的 `0xB1` decoder semantics 或 CI evidence。

**Execution / validation：**
- Fresh-sync current `main` 後重新確認 defect 仍存在。
- 僅修改 `docs/roadmap.md` 的該 relative link target。
- 驗證新 relative target 對應現存 repository path。
- `git diff --check` PASS。
- 不需為純 link 修正執行 ESP-IDF compile/link；若 current governance/CI 自動執行則照常觀察，不把不必要 build 當 completion blocker。
- 確認 final diff 僅 `docs/roadmap.md` 一行 link target。
- Library readiness = `PRESERVED`。
- 成功後把 `TASKS.md` 收斂回 minimal `EMPTY`，commit/push，做 canonical remote read-back。

**Hard exclusions：**
- Do not alter the surrounding roadmap claim or `0xB1` semantics.
- Do not modify source/tests/spec/README.
- Do not execute adjacent Cold work.
