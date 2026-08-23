# Architecture

`Phase 1C — WF-RAC Transport Abstraction` 已開始；目前實作 portable WF-RAC Protocol Core #1–#6、Device Model foundations、portable transport contract、HTTPS-first scheme policy，以及 portable TimeProvider／typed API request builders。Time Source → TimeProvider → WF-RAC API Envelope → Transport；AirconStat Codec → Base64 → API Envelope。Transport 是最底層 opaque byte/text boundary，不依賴 Protocol、Device Model 或 Matter；其 request／response body 不知道 AirconStat、Base64、JSON schema、identity 或 timestamp semantics。HTTP status 是上層 application data，不等同 transport error。ESP-IDF adapter 已實作 bounded synchronous single-attempt POST 與 caller-owned response collection，但本 task 沒有執行任何 network I/O。

專案保持 monorepo，但 reusable portable library components 是 `wfrac_protocol`、`wfrac_device_model`、`wfrac_api`、`wfrac_transport` 與 `wfrac_client`；`wfrac_transport_espidf` 是唯一 ESP-IDF platform adapter，依賴 portable transport 與 ESP-IDF HTTP diagnostics。`firmware/idf_compile_smoke` 只是 reference build consumer，不是 portable component 的隱性 dependency。library packaging、versioning 與外部 distribution 尚未開始。

```text
WF-RAC Transport (HTTP :51443 / HTTPS :51443)
        ↓
WF-RAC API Envelope → AirconStat Codec → WF-RAC Device Model
        ↓
Matter Adapter → Matter over Wi-Fi
```

Transport attempt 必須明確指定 HTTP 或 HTTPS；legacy firmware 可能使用 HTTP，newer WF-RAC-HTTPS firmware 可能使用 HTTPS。較新的上游行為包含 self-signed certificate、TLS 1.2、ALPN `http/1.1`、`Connection: close` 與一次只允許單一連線的限制；這些不是所有 firmware 的通用要求。Portable `SchemePolicy` 以 caller-owned per-device cache 選出明確 `TransportScheme`，再由 `ITransport` 執行一次 physical attempt；unknown 採 HTTPS-first，只有明確 `scheme_mismatch` 可回傳一次 HTTP compatibility fallback decision，TLS identity failure 與 transient failure 均禁止 downgrade，confirmed scheme 維持 sticky。`EspHttpTransport` 在 platform adapter 以 fresh ESP-IDF client 執行 bounded synchronous HTTP POST，關閉 redirect／authorization retry／keep-alive，並要求 HTTPS non-empty trust certificate；它不讀寫 SchemeCache、不含 loop／retry，也不推定 fallback。失敗流程為 API bytes → `TransportRequest` → `EspHttpTransport` → `esp_http_client_perform` → `EspHttpFailureEvidence` → classifier → `TransportError` → external `SchemePolicy`。verify flags 為 `tls_identity_failure`、generic TLS diagnostics 為 `tls_failure`、已知 timeout 為 `timeout`，而 local invalid transport 為 `invalid_argument`；classifier 永不輸出 `scheme_mismatch`。persistent cache、rediscovery trigger、actual WF-RAC TLS/network validation 與 runtime 仍 pending。API envelope 已用 caller-owned buffer實作五個 typed local JSON builders；它會以 64-bit TimeProvider sample 建立 timestamp、escape opaque IDs，並在 Base64 wrapping 前驗證 AirconStat 結構與兩個 CRC。registration builders 固定從 context operatorId 取得 accountId，update 使用 compatibility `remote=0`；這些仍只是 request serialization，不包含 Operator ID persistence、lifecycle、response parsing、envelope dispatch 或 runtime。

```text
per-device SchemeCache
        ↓
SchemePolicy selects explicit HTTP or HTTPS
        ↓
ITransport::perform() executes ONE physical attempt
```

```text
ApiRequestView
        ↓
wfrac_client request executor
        ↓
SchemePolicy
        ↓
ITransport
        ↓
platform adapter
```

`wfrac_client` 不 serialize JSON、不建 timestamp、不 parse response，也不實作 HTTP/TLS。它將已建立的 `ApiRequestView` 以原 body 交給首次 attempt，立即以 SchemePolicy 記錄結果；只有 policy 明確回傳 `retry_http_compatibility` 才以同一 body 作唯一 HTTP fallback。因此每次 executor 最多兩次 physical attempts，沒有 loop 或 automatic rediscovery。

raw HTTP response bytes → `wfrac_api` response envelope → command/result/contents → typed `getDeviceInfo` bootstrap fields。response parser 不依賴 HTTP status、Transport 或 ESP-IDF；nonzero result 只是保留的 command-specific data，不是 generic parse failure。`getAirconStat` contents parser 以 caller-owned scratch 將 JSON-decoded Base64 text 交給 `wfrac_protocol::base64_decode`，再以 `parse_aircon_stat` 驗證結構與 COMMAND／RECEIVE CRC；只有兩個 CRC 都有效才輸出 `AirconFrameView`，供未來 Protocol semantic／Device Model integration 使用。

```text
WF-RAC API bytes
        ↓
TransportRequest
        ↓
EspHttpTransport
        ↓
esp_http_client_perform
        ↓
EspHttpFailureEvidence
        ↓
ESP-IDF error classifier
        ↓
TransportError
        ↓
external SchemePolicy
```

Matter Adapter 只對 Device Model 發出高階 request，如 `setPower()`、`setMode()`、`setTargetTemperature()`、`setFanMode()`、`requestStatus()`；不直接處理 HTTP、JSON、Operator ID、retry、discovery 或 socket。Device Model 應與傳輸層及上層整合介面解耦，保持清楚的模組邊界與可測試性。

Device Model 目前明確區分最近設備 observation、上層 desired writable state、尚未由外層 resolve 的單一 pending command，以及外層設定的 availability。設定 desired 或 pending 不會 optimistic 更新 observed；pending completion 也不會自行改寫 observed 或 desired。`reconcile_observation()` 可明確以新 observation 判定 pending target 是否 observed-converged，並逐欄清除已收斂的 desired；若 active pending 與同欄位較新的 desired 不同，該 desired 會保留以避免 race。這只代表 observation 與 target 一致，不代表 transport confirmation 或 command causality。明確的 command mapping boundary 會將單一 Device Model intent 轉成 Protocol safe `CommandState`，並僅接受可 lossless 表示的 0.5°C target temperature。observed-baseline request-composition boundary 進一步使用最新 CRC-valid RECEIVE state 作 compatibility baseline、只 patch requested receive-side field、carry COMMAND byte8 bit `0x08`，並為兩個 block 放入 official no-request sentinel；它輸出 raw AirconStat，仍不組合 Base64、JSON 或實際 request。full safe operation-data query request composition 也使用相同 baseline／byte8 carry policy，並在 COMMAND 放入 1–3 個 allowlisted query segments、在 RECEIVE 放入 no-request sentinel；它仍未建立 API envelope、automatic command dispatch、queue、polling、timeout source 或 runtime。

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
