# Unfinished work queue

## Hot — Add diagnostic-only Web UI for read-only WF-RAC probe

**Goal：** 在 `firmware/wfrac_bootstrap_probe` 增加一個 browser-based diagnostic UI，讓使用者透過 ESP32 temporary SoftAP 提供一次性測試參數，再由 ESP32 以 APSTA 模式執行單次、bounded、read-only `getDeviceInfo`。這不是正式 Provisioning/WebUI。

**Scope：**
- Web UI 僅屬 diagnostic firmware；portable components保持解耦。
- Default build保持 inert；新增明確、default-off 的 diagnostic WebUI build switch。
- WebUI enabled 時使用 ESP-IDF `esp_http_server` + temporary SoftAP/APSTA。
- UI assets全部嵌入 firmware；不使用外部 CDN、filesystem、captive portal、mDNS或NVS。
- 表單只提供完成一次 read-only probe 所需的 Wi-Fi / WF-RAC / time / explicit scheme / HTTPS trust inputs；使用 bounded POST，不用 query-string承載敏感輸入。
- 一次 submit最多觸發一次 bounded STA connect與一次 read-only `getDeviceInfo`；不做 polling、auto retry、write command或registration mutation。
- Result只顯示 sanitized stage/result資訊；不得回顯或記錄敏感輸入。
- Wi-Fi runtime維持 RAM-only；HTTPS仍要求 explicit trust，禁止 insecure TLS或自動降級。
- 完成或失敗後清理 STA/probe state，但 diagnostic SoftAP/WebUI可保留供下一次使用者明確操作。
- 優先重構既有 one-shot probe為 firmware-local reusable executor/result，避免複製 request logic。

**Governance reconciliation：**
- Current `AGENTS.md` 對 WebUI / Wi-Fi runtime / credential handling 的 generic prohibition需要最小修正，只允許本 Stage 的 diagnostic-only RAM-bound exception；production Provisioning/WebUI、persistent credential store與一般 runtime仍禁止。
- 必要時同步 `docs/security.md`、`docs/architecture.md`、`docs/roadmap.md`、`README.md`，明確區分 diagnostic compile-ready harness 與未來 production WebUI。
- Hardware/network execution仍是獨立 gate；本 Stage只 implementation + compile/host validation，不執行真實 Wi-Fi/WF-RAC測試。
- Remaining Cold Candidate `First real WF-RAC read-only probe / hardware validation` 保持 Cold，不在本 Stage執行或移除。

**Validation：**
- default/inert configuration維持無 diagnostic network activity。
- WebUI-enabled configuration在 ESP-IDF v6.0.2 下對 ESP32-C6與ESP32-C3 compile/link PASS。
- Continuous ESP-IDF CI需實際覆蓋 WebUI-enabled build path；existing host/software CI維持 green。
- 以可離線測試的 seam驗證 input bounds、required fields、explicit scheme/TLS guard、secret non-reflection等關鍵 invariant。
- 不在 CI 或本 Stage執行 firmware/network/hardware。
- Library readiness = PRESERVED。
- `git diff --check` PASS；README derived metrics若受影響需與 final tracked content一致。
- 完成後 `TASKS.md` 回到 `EMPTY`，commit/push並取得 remote read-back與 C6/C3 WebUI-enabled compile evidence。
