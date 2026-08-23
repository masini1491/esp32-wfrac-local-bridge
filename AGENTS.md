# Development discipline

本 repository 目前進入 Phase 1C — WF-RAC Transport Abstraction。允許既有 Phase 1A protocol core 與 Phase 1B Device Model 維護、portable transport contracts、request／response DTO、portable response envelope／getDeviceInfo bootstrap parsing、getAirconStat JSON／Base64／AirconStat integrity parsing、validated Protocol frame → Device Model observation／reconciliation mapping、FakeTransport／host-side unit tests、endpoint／scheme value types、bounded timeout value representation、portable API／transport orchestration、ESP-IDF-specific transport adapter、HTTP/TLS error classifier、bounded synchronous single-attempt HTTP POST／response collection 與 ESP32-C6／ESP32-C3 compile/link integration。仍禁止實際 HTTP/HTTPS request execution、TLS handshake、Wi-Fi runtime、mDNS runtime、NVS、real WF-RAC requests、scheme retry／polling runtime、hardware/network test、Matter Adapter／Matter runtime、MQTT、WebUI、credential handling，以及未經要求的 scope expansion。

正式 framework 是 ESP-IDF；ESP32-C6 為 primary、ESP32-C3 為 compatibility target；Matter-over-Wi-Fi 是 primary transport。固定 layering：WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi。

Matter Adapter 不得直接建 request、parse raw JSON、管理 Operator ID、retry、discovery 或 socket；Device Model 應與傳輸層及上層整合介面解耦，保持清楚的模組邊界與可測試性。未來 runtime 應採 non-blocking、bounded timeout/retry、bounded command queue、command serialization、polling/command coordination、state consistency 與 explicit availability。

使用 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING`，並記 repository、revision、license 與研究用途。不可未審 license copy、line-by-line translation、mechanical port 或 rename-and-claim。`mhi-ac-ctrl-esp32` 不是 WF-RAC HTTP protocol authority。

日常控制不得依賴雲端、MQTT、Internet、DNS 或 NTP。不得提交或輸出 Wi-Fi credentials、Operator ID、fabric/commissioning secrets、OTA signing key 或 WebUI secrets。加密 NVS、Secure Boot、Flash Encryption 目前只列 roadmap。

失敗分類：`SOURCE`、`TOOLCHAIN`、`ENVIRONMENT`、`INFRASTRUCTURE`、`SERVICE`、`HARDWARE_REQUIRED`。同一 infrastructure/auth/toolchain/service root cause 最多自動 retry 1 次；第二次停止，不重掃、不重複 download、不因 toolchain 問題修改 production source。沒有明確要求不得自動實作；保留 hardware truth，避免宣稱未驗證能力。
## Reference reading discipline

Reference reading must be topic-scoped。Future task 先讀 `docs/references/README.md`，再只讀直接相關的 `topics/*.md`；只有 provenance、source conflict 或 evidence audit 才讀 `sources/*.md`。不得預設完整讀 `docs/protocol-research.md` 或全部 reference files；evidence 不足時才逐層擴大 context。
