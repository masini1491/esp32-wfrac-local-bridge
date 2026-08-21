# esp32-wfrac-local-bridge

WF-RAC → Matter 的本地研究與架構 baseline。

## Project status

目前是 **Phase 0 — Research / Architecture / Project Setup**。本 repository 尚未包含 firmware，刻意延後 firmware development 與硬體驗證。

- Framework：ESP-IDF
- Primary MCU：ESP32-C6
- Compatibility MCU：ESP32-C3
- Primary transport：Matter over Wi-Fi
- WF-RAC：本地 HTTP/HTTPS API（transport 尚未實作）
- 控制原則：Local-first，不依賴雲端、MQTT 或 Internet 作為日常控制生命線

架構方向為 `WF-RAC Transport → WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi`。Matter Adapter 只使用高階 Device Model request，不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。

YDM/Yale 與 Door Access 不屬於本 repository。未來將成熟 WF-RAC backend 重用於 Private Universal Matter Bridge 只是規劃，不代表本專案現在建立 universal framework 或 bridge。

## Scope and evidence

本階段只記錄研究、架構、硬體策略、安全與 roadmap。Reference facts 依 `CONFIRMED_UPSTREAM`、`CONFIRMED_LOCAL`、`INFERRED`、`UNKNOWN`、`HARDWARE_TEST_PENDING` 分級。WF-RAC 使用者硬體的 protocol/runtime validation 目前為 `HARDWARE_TEST_PENDING`。

## License

TBD。正式公開前再決定 project license；本 baseline 不新增 LICENSE。

詳見 [docs/roadmap.md](docs/roadmap.md)。
