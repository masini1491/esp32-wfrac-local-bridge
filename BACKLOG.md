# Cold Registry

Cold coordination only. This surface has **no execution authority**: a Cold item needs current authority/evidence reconciliation, promotion to the Sole Hot in [TASKS.md](TASKS.md), and separate user launch approval. It is not canonical architecture/protocol/evidence truth.

## Committed sequence — current position

- **Stage 1 (PR #6)** and **Stage 2 (PR #7)** were already accepted within their historical software/CI evidence scopes; do not keep their execution contracts in the active or Cold queue. Exact former descriptions and Git pins are preserved in [BACKLOG.md@6b66a469](https://github.com/masini1491/esp32-wfrac-local-bridge/blob/6b66a469f2683800939e8ab98e460dbdf9cac63a/BACKLOG.md).
- **Stage 3 / `GATEA-D2`** is **temporarily parked / blocked** on the original Codex WSL launcher and independent raw-receipt prerequisites after `PORTABLE-HDR-001` was accepted and merged in [PR #21](https://github.com/masini1491/esp32-wfrac-local-bridge/pull/21); `PORTABLE-BUILD-001` was accepted and merged in [PR #22](https://github.com/masini1491/esp32-wfrac-local-bridge/pull/22) with post-merge Host CI SUCCESS; there is currently no admitted Hot. Stage 3 remains parked and no next Stage is automatically launched. No Gate A progress, test-acceptance or original A3 authorization has been upgraded/revoked; the A3 start counter remains **0/1**. Preserve the full existing Stage 3 contract and D3/D4/history at [pre-pause TASKS.md](https://github.com/masini1491/esp32-wfrac-local-bridge/blob/da8e508f9a475fd5a13c5b638f363c06556e3785/TASKS.md); before restarting, reconcile current governance and require fresh scope-appropriate execution admission.
- **Stage 4** remains **COLD**, still dependent on formal Stage 3 closure, fresh evidence/source review and separate promotion/authorization; `PORTABLE-HDR-001` does not satisfy that dependency. Earlier sequencing authorization does not launch it.

## COMMITTED — Stage 4 (COLD; not admitted to execution)

- **Stage 4 — README evidence-scope and tracked-line counts (P3).** Update `README.md` to distinguish 2026-09-02 Windows PC local `CONFIRMED_LOCAL` observations (two real modules, sanitized evidence in `docs/references/sources/local-wfrac-pc-validation-2026-09-02.md`) from ESP32 runtime, physical HVAC actuation and universal `HARDWARE_TEST_PENDING`. Do **not** alter `specs/wf-rac-contracts.json` `current_confirmed_local_count=0` merely because PC evidence exists; it explicitly represents narrower machine-validation scope. Reproduce README tracked physical-line categorization and exclusions at the then-current exact Git tree; health audit reported prior 12,567 → computed 13,690 at its audited revision (Firmware 6120 / tools+tests+workflow 6137 / contracts 440 / technical docs 960 / other 33), but **recompute** at Stage 4 because earlier stages change line counts. Prefer reproducible/documented formula over evergreen totals; minimize new tooling. Host contract/doc links + `git diff --check`; no hardware.

The reported historical tracked-line counts and prior CI evidence remain revision-scoped. Recompute at Stage 4's actual future target revision; no automatic README/spec/source change is allowed now.

## CANDIDATE — Other newly evidenced software-only residuals

- Only admit when new current upstream/local evidence creates a separate bounded, verifiable software-only tranche.
- Do not manufacture speculative decoder/control work for completeness.

## CANDIDATE — First real WF-RAC read-only probe / hardware validation

- Goal：在明確 target/board/FQBN/device identity、endpoint/scheme/time seed/trust inputs與使用者 hardware/network authorization成立後，執行 bounded、read-only `getDeviceInfo` probe。
- Trigger / scope：先讀 current Git/AGENTS/TASKS/evidence、diagnostic firmware、classifier/parser與直接 caller；只使用 one-shot read-only path，收集最小必要 target evidence。
- Existing evidence：2026-09-02 PC observed profile 可作 plain HTTP `:51443` read-only baseline；不構成 ESP32 hardware/network PASS，也不建立通用 scheme、HTTPS 或 firmware 行為。
- Exclusions：不得 insecure TLS、auto downgrade、write command、registration、polling、Matter；不得輸出或保存 credentials/operator/cert material。Hardware/network authorization不自動授權 repository mutation。
- Completion gate：真實 ESP32 target取得符合 evidence contract的 read-only `getDeviceInfo`結果；失敗依 evidence分類，不猜 root cause。後續 `getAirconStat`另行 admission。
