# Architecture

`Phase 1B — WF-RAC Device Model` 已開始；目前實作 portable WF-RAC Protocol Core #1–#6、基本 HVAC state projection、tri-state app capability hints，以及 observed／desired／single-pending command／availability 的 state ownership skeleton。Device Model state ownership 不依賴 protocol types，不處理 raw AirconStat、transport、runtime、hardware 或 Matter implementation；protocol semantic types 只在 protocol mapping 邊界使用。

```text
WF-RAC Transport (HTTP :51443 / HTTPS :51443)
        ↓
WF-RAC API Envelope → AirconStat Codec → WF-RAC Device Model
        ↓
Matter Adapter → Matter over Wi-Fi
```

Transport 不預設為 plain HTTP；legacy firmware 可能使用 HTTP，newer WF-RAC-HTTPS firmware 可能使用 HTTPS。較新的上游行為包含 self-signed certificate、TLS 1.2、ALPN `http/1.1`、`Connection: close` 與一次只允許單一連線的限制；這些不是所有 firmware 的通用要求。本階段不建立 transport implementation。API envelope 的 `airconStat` 是 Base64 binary payload；portable Base64、CRC16、block serialization 與 generic full AirconStat composition 已在 Protocol Core 實作，但 request RECEIVE policy、JSON envelope 與 runtime 尚未建立。

Matter Adapter 只對 Device Model 發出高階 request，如 `setPower()`、`setMode()`、`setTargetTemperature()`、`setFanMode()`、`requestStatus()`；不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。Device Model 應與傳輸層及上層整合介面解耦，保持清楚的模組邊界與可測試性。

Device Model 目前明確區分最近設備 observation、上層 desired writable state、尚未由外層 resolve 的單一 pending command，以及外層設定的 availability。設定 desired 或 pending 不會 optimistic 更新 observed；pending completion 也不會自行改寫 observed 或 desired。`reconcile_observation()` 可明確以新 observation 判定 pending target 是否 observed-converged，並逐欄清除已收斂的 desired；若 active pending 與同欄位較新的 desired 不同，該 desired 會保留以避免 race。這只代表 observation 與 target 一致，不代表 transport confirmation 或 command causality。command → protocol encoding、automatic command dispatch、queue、polling、timeout source 與 runtime 仍未建立。

## WF-RAC module 與 AirconStat 的來源關係

新增 pinned reference 的 firmware analysis 將 WF-RAC module 描述為雙處理器架構：

```text
Realtek RTL8710C
        ↓ UART
Renesas RL78
        ↓ CNS / SPI
MHI AC main board
```

Realtek side 負責 Wi-Fi、HTTP／HTTPS、MQTT、TLS、Base64、OTA 與 network plumbing；RL78 side 負責 CNS／SPI protocol bridge、AirconStat 與 MHI CNS/SPI frame 的轉換，以及 remote-control role。這表示 WF-RAC network API 的 AirconStat 並非與 MHI CNS/SPI 完全無關的獨立格式，而是 JSON／Base64 transport representation → AirconStat → CNS/SPI-derived state／control representation。

這是 protocol semantics 的 provenance view，不是本 project 新增 CNS／SPI runtime layer。ESP32 仍透過 Local IP 與 WF-RAC module 通訊，再由 module 連接 MHI air conditioner。

Direct CNS/SPI capability 不等於 WF-RAC network-visible capability。CNS/SPI cross-reference 只用於 field identification、operation-data address identification 與 set-bit understanding；runtime 仍只與 WF-RAC local network API 通訊。

WF-RAC 的本地控制需要 Wi-Fi/IP network，因此 Matter over Wi-Fi 是 primary Matter transport；Matter over Thread 不是本 project primary target。未來 runtime 必須採 non-blocking 架構、bounded timeout/retry、command serialization、polling/command coordination、no infinite retry、no unbounded queue 與 explicit availability。

## Matter 參考與 mapping 邊界

Pinned `esp-matter` reference 支援 Room Air Conditioner device type `0x0072`、version `3`，包含 Descriptor、Identify、On/Off、Thermostat；On/Off 另有 `dead_front_behavior` feature。generated data model 在上游文件中標示 Experimental。完整 WF-RAC mapping 尚未完成。

Fan speed、fan-only、dry、horizontal/vertical direction、3D Auto、outdoor temperature、energy、vacant-property mode、error code 的 Matter mapping 均為 `UNKNOWN / Phase 0.3 pending`。目前不決定 custom cluster、separate endpoint、Fan/Temperature Sensor device 或 vendor-specific cluster。
