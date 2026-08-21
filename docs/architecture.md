# Architecture

`Phase 0 — Research / Architecture / Project Setup`；以下是 planned architecture，不是已完成 runtime。

```text
WF-RAC Transport (HTTP :51443 / HTTPS :51443, self-signed certificate)
        ↓
WF-RAC Protocol → WF-RAC Device Model → Matter Adapter → Matter over Wi-Fi
```

Transport 不預設為 plain HTTP；legacy firmware 可能使用 HTTP，newer WF-RAC-HTTPS firmware 可能使用 HTTPS。API path family 為 `/beaver/command/<command>`，但本階段不建立 implementation。

Matter Adapter 只對 Device Model 發出高階 request，如 `setPower()`、`setMode()`、`setTargetTemperature()`、`setFanMode()`、`requestStatus()`；不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。Device Model 保持與 Matter、MQTT、WebUI、雲端平台無關。

WF-RAC local control 需要 Wi-Fi/IP network，因此 Matter over Wi-Fi 是 primary Matter transport；Matter over Thread 不是本 project primary target。未來 runtime 必須 non-blocking、bounded timeout/retry、command serialization、polling/command coordination、no infinite retry、no unbounded queue 與 explicit availability。
