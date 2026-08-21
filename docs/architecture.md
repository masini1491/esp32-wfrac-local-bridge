# Architecture

`Phase 0 — Research / Architecture / Project Setup`；以下是 planned architecture，不是已完成 runtime。

```text
WF-RAC Transport (HTTP :51443 / HTTPS :51443)
        ↓
WF-RAC API Envelope → AirconStat Codec → WF-RAC Device Model
        ↓
Matter Adapter → Matter over Wi-Fi
```

Transport 不預設為 plain HTTP；legacy firmware 可能使用 HTTP，newer WF-RAC-HTTPS firmware 可能使用 HTTPS。較新 upstream behavior 包含 self-signed certificate、TLS 1.2、ALPN `http/1.1`、`Connection: close` 與 single-connection constraint；這些不是所有 firmware 的 universal requirement。本階段不建立 implementation。API envelope 的 `airconStat` 是 Base64 binary payload，另有 CRC16-CCITT codec；本階段不建立 codec。

Matter Adapter 只對 Device Model 發出高階 request，如 `setPower()`、`setMode()`、`setTargetTemperature()`、`setFanMode()`、`requestStatus()`；不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。Device Model 保持與 Matter、MQTT、WebUI、雲端平台無關。

WF-RAC local control 需要 Wi-Fi/IP network，因此 Matter over Wi-Fi 是 primary Matter transport；Matter over Thread 不是本 project primary target。未來 runtime 必須 non-blocking、bounded timeout/retry、command serialization、polling/command coordination、no infinite retry、no unbounded queue 與 explicit availability。

## Matter reference and mapping boundary

Pinned `esp-matter` reference 支援 Room Air Conditioner device type `0x0072`, version `3`，包含 Descriptor、Identify、On/Off、Thermostat，On/Off 另有 `dead_front_behavior` feature；generated data model 在 upstream 文件中標示 Experimental。完整 WF-RAC mapping 尚未完成。

Fan speed、fan-only、dry、horizontal/vertical direction、3D Auto、outdoor temperature、energy、vacant-property mode、error code 的 Matter mapping 均為 `UNKNOWN / Phase 0.3 pending`。目前不決定 custom cluster、separate endpoint、Fan/Temperature Sensor device 或 vendor-specific cluster。
