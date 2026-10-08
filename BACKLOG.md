# Cold Registry

本檔是 Cold coordination surface：ordinary bootstrap 預設不載入、不具 execution authority，也不是 architecture、protocol、security 或 evidence canonical truth。Cold item 必須先回到 current authority/evidence reconciliation，promotion 到 `TASKS.md` Hot coordination，並取得使用者明確 launch 才能執行；保存不提高 AI 建議的 recommendation authority。

## COMMITTED — Software-only continuation sequence

使用者已明確要求依序執行本 sequence。**Docs reconciliation 已 promotion 為 current Hot**；本 Cold 只保留其後尚未 promotion 的 committed stages。每次仍只可對下一個 bounded Stage 做 fresh authority/evidence reconciliation，promotion 到 `TASKS.md`，完成 validation/closure 後才進下一項。

1. **Vertical vane protocol core**
   - Fresh-reconcile pinned WF-RAC evidence for vertical vane position / up-down swing receive + command set-bit semantics.
   - Admit only the bounded portable protocol / request-composition / regression surface that current evidence supports.
   - Hardware actuation remains `HARDWARE_TEST_PENDING`.

2. **Horizontal vane / 3D Auto read-side**
   - Prefer read-side semantic/typed observation work first for `state[11]` / `state[12]`.
   - Do not infer or admit write semantics unless fresh evidence is independently sufficient.
   - Preserve model/capability gating and `HARDWARE_TEST_PENDING`.

3. **Advanced fixed-state guards**
   - Reconcile bounded read-side semantics / negative guards for remaining advanced fixed-state fields such as Vacant, CoolHotJudge, error/self-clean-related observations where evidence is sufficient.
   - Do not turn unreliable/status-only evidence into control support.

4. **Device Model / Matter mapping**
   - Only after lower protocol/read-side semantics are stable, reconcile bounded software-only Device Model and SDK-free Matter adapter mappings.
   - Actual Matter runtime, node/endpoint execution, commissioning, interoperability, hardware/network remain out of scope.

Common rules:
- Sequence is durable future work, not a batch launch.
- One bounded tranche at a time: `Cold → fresh reconcile → Hot promotion → explicit launch → validation → closure`.
- Do not perform exploratory write, real Wi-Fi/HTTP/TLS/WF-RAC/hardware execution, or promote software evidence to `CONFIRMED_LOCAL`.
- Do not invent semantics for completeness; unresolved evidence remains UNKNOWN / unsupported / pending as applicable.

## CANDIDATE — Other newly evidenced software-only residuals

- Only admit when new current upstream/local evidence creates a separate bounded, verifiable software-only tranche.
- Do not manufacture speculative decoder/control work for completeness.

## CANDIDATE — First real WF-RAC read-only probe / hardware validation

- Goal：在明確 target/board/FQBN/device identity、endpoint/scheme/time seed/trust inputs與使用者 hardware/network authorization成立後，執行 bounded、read-only `getDeviceInfo` probe。
- Trigger / scope：先讀 current Git/AGENTS/TASKS/evidence、diagnostic firmware、classifier/parser與直接 caller；只使用 one-shot read-only path，收集最小必要 target evidence。
- Existing evidence：2026-09-02 PC observed profile 可作 plain HTTP `:51443` read-only baseline；不構成 ESP32 hardware/network PASS，也不建立通用 scheme、HTTPS 或 firmware 行為。
- Exclusions：不得 insecure TLS、auto downgrade、write command、registration、polling、Matter；不得輸出或保存 credentials/operator/cert material。Hardware/network authorization不自動授權 repository mutation。
- Completion gate：真實 ESP32 target取得符合 evidence contract的 read-only `getDeviceInfo`結果；失敗依 evidence分類，不猜 root cause。後續 `getAirconStat`另行 admission。
