# Cold Registry

本檔是 Cold coordination surface：ordinary bootstrap 預設不載入、不具 execution authority，也不是 architecture、protocol、security 或 evidence canonical truth。Cold item 必須先回到 current authority/evidence reconciliation，promotion 到 `TASKS.md` Hot coordination，並取得使用者明確 launch 才能執行；保存不提高 AI 建議的 recommendation authority。

## CANDIDATE — Remaining software-only protocol / diagnostic work

`0x81/0x87` bounded indoor-coil temperature candidate decoder 已完成並從 Hot 收斂；以下只保留尚未 admission 的 software-only residual。

建議後續依下列順序逐項 fresh-reconcile、一次 promotion 一個 bounded tranche：

1. **`0x82` THO-R1 typed raw decoder**
   - shared outdoor。
   - 只暴露 exact raw/network byte與完整 raw segment。
   - physical unit維持 UNKNOWN；不得重用 `0x81/0x87` indoor-coil curve。

2. **`0xB1` TDSH-related typed raw decoder**
   - shared outdoor。
   - 只暴露 exact raw/network byte與完整 raw segment。
   - Celsius conversion目前未建立；不得直接採 secondary direct-CNS/SPI formula。

3. **Residual semantic tightening**
   - `0x7C`：維持 UNKNOWN，不建模為 protection boolean。
   - `0x1E/0x1F`：維持 confirmed response-no-value / candidate semantics，不把 no-value誤判為 universally unsupported。
   - `0x0D/0x21/0x32/0x34/0x35` 等 unknown semantic codes：只有 current evidence足以縮小語意時才另分 tranche；UNKNOWN 維持 UNKNOWN。
   - 特別保留 `0x34` 使用 OP3 的 multi-byte evidence，不套單-byte慣例。

4. **Vane / horizontal vane / 3D Auto / advanced fixed-state**
   - 功能價值高，但涉及 control/write semantics 與不同 completion gate。
   - 必須先 fresh-reconcile pinned evidence、command/receive encoding與安全邊界，再決定是否拆成一個或多個 Hot Stage。
   - 不得因 read-side evidence存在就推定 write semantics已驗證。

5. **Other newly evidenced software-only residuals**
   - 只有出現新的 current upstream/local evidence，且能形成 bounded、可驗證 tranche時才 admission。
   - 不為完整度製造 speculative decoder／control。

共通 exclusions：不做 exploratory write、不把 direct CNS/SPI semantics直接升成 WF-RAC network truth、不執行硬體/network、不把 software test升級為 `CONFIRMED_LOCAL`。

## CANDIDATE — First real WF-RAC read-only probe / hardware validation

- Goal：在明確 target/board/FQBN/device identity、endpoint/scheme/time seed/trust inputs與使用者 hardware/network authorization成立後，執行 bounded、read-only `getDeviceInfo` probe。
- Trigger / scope：先讀 current Git/AGENTS/TASKS/evidence、diagnostic firmware、classifier/parser與直接 caller；只使用 one-shot read-only path，收集最小必要 target evidence。
- Existing evidence：2026-09-02 PC observed profile 可作 plain HTTP `:51443` read-only baseline；不構成 ESP32 hardware/network PASS，也不建立通用 scheme、HTTPS 或 firmware 行為。
- Exclusions：不得 insecure TLS、auto downgrade、write command、registration、polling、Matter；不得輸出或保存 credentials/operator/cert material。Hardware/network authorization不自動授權 repository mutation。
- Completion gate：真實 ESP32 target取得符合 evidence contract的 read-only `getDeviceInfo`結果；失敗依 evidence分類，不猜 root cause。後續 `getAirconStat`另行 admission。
