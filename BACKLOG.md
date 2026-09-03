# Cold Registry

本檔是 Cold coordination surface：ordinary bootstrap 預設不載入、不具 execution authority，也不是 architecture、protocol、security 或 evidence canonical truth。Cold item 必須先回到 current authority/evidence reconciliation，promotion 到 `TASKS.md` Hot coordination，並取得使用者明確 launch 才能執行；保存不提高 AI 建議的 recommendation authority。

## CANDIDATE — Machine-readable TimeProvider implementation-state reconciliation

- Goal：校正 machine-readable contract 與目前 portable `TimeProvider` boundary／ESP-IDF adapter implementation state，同時保留 trusted-time acquisition、persistence、SNTP/NTP、power-loss recovery 與 production runtime 尚未完成。
- Trigger / scope：重新讀取 current contract、validator、time provider source／docs 後，做 bounded vocabulary、validator/tests/docs consistency maintenance；不得宣稱未完成的 acquisition 或 persistence。
- Exclusions：不新增 SNTP/NTP、network dependency、NVS persistence、power-loss recovery 或 production runtime。
- Completion gate：machine-readable state 能表達 provider/time-quality boundary 已實作，並保留 acquisition/persistence/runtime pending；targeted validator/tests/docs PASS。

## CANDIDATE — Continuous ESP-IDF C6/C3 compile/link CI gate

- Goal：建立 ESP-IDF v6.0.2 platform integration 與 diagnostic firmware 的 C6 primary／C3 compatibility compile/link CI gate。
- Trigger / scope：重新確認 current workflow、firmware CMake 與 toolchain authority後，建立 bounded CI provisioning/cache/matrix；只做 compile/link。
- Exclusions：不做 hardware、Wi-Fi、HTTP/HTTPS、TLS handshake、real WF-RAC，也不提交 credentials/operator/cert material；既有 host validation保持不變。
- Completion gate：CI 可區分 toolchain/config/source failure，C6/C3 compile/link PASS，且成本與 scope 維持 bounded。

## CANDIDATE — First real WF-RAC read-only probe / hardware validation

- Goal：在明確 target/board/FQBN/device identity、endpoint/scheme/time seed/trust inputs與使用者 hardware/network authorization成立後，執行 bounded、read-only `getDeviceInfo` probe。
- Trigger / scope：先讀 current Git/AGENTS/TASKS/evidence、diagnostic firmware、classifier/parser與直接 caller；只使用 one-shot read-only path，收集最小必要 target evidence。
- Existing evidence：2026-09-02 PC observed profile 可作 plain HTTP `:51443` read-only baseline；不構成 ESP32 hardware/network PASS，也不建立通用 scheme、HTTPS 或 firmware 行為。
- Exclusions：不得 insecure TLS、auto downgrade、write command、registration、polling、Matter；不得輸出或保存 credentials/operator/cert material。Hardware/network authorization不自動授權 repository mutation。
- Completion gate：真實 ESP32 target 取得符合 evidence contract 的 read-only `getDeviceInfo` 結果；失敗依 evidence 分類，不猜 root cause。後續 `getAirconStat` 另行 admission。
