# Cold Registry

本檔是 Cold coordination surface：ordinary bootstrap 預設不載入、不具 execution authority，也不是 architecture、protocol、security 或 evidence canonical truth。Cold item 必須先回到 current authority/evidence reconciliation，promotion 到 `TASKS.md` Hot coordination，並取得使用者明確 launch 才能執行；保存不提高 AI 建議的 recommendation authority。

## CANDIDATE — Advanced protocol / diagnostic semantics

- Goal：繼續補齊 current evidence可支持的 vane、diagnostics、advanced fixed-state fields、operation-data physical conversions與其他 read-side semantics。
- Trigger / scope：topic-scoped fresh-read current references/protocol authority；只對有足夠 evidence的 field admission，UNKNOWN維持 UNKNOWN。
- Exclusions：不做 exploratory write、不從 CNS/SPI semantics直接宣稱 WF-RAC network write已驗證、不執行硬體/network。
- Completion gate：新增 semantics有 provenance、machine/host tests與bounded claims，沒有越過 evidence等級。

## CANDIDATE — First real WF-RAC read-only probe / hardware validation

- Goal：在明確 target/board/FQBN/device identity、endpoint/scheme/time seed/trust inputs與使用者 hardware/network authorization成立後，執行 bounded、read-only `getDeviceInfo` probe。
- Trigger / scope：先讀 current Git/AGENTS/TASKS/evidence、diagnostic firmware、classifier/parser與直接 caller；只使用 one-shot read-only path，收集最小必要 target evidence。
- Existing evidence：2026-09-02 PC observed profile 可作 plain HTTP `:51443` read-only baseline；不構成 ESP32 hardware/network PASS，也不建立通用 scheme、HTTPS 或 firmware 行為。
- Exclusions：不得 insecure TLS、auto downgrade、write command、registration、polling、Matter；不得輸出或保存 credentials/operator/cert material。Hardware/network authorization不自動授權 repository mutation。
- Completion gate：真實 ESP32 target取得符合 evidence contract的 read-only `getDeviceInfo`結果；失敗依 evidence分類，不猜 root cause。後續 `getAirconStat`另行 admission。
