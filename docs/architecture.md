# Architecture

`Phase 1C — WF-RAC Transport Abstraction` 已開始；目前實作 portable WF-RAC Protocol Core #1–#6、Device Model foundations、portable transport contract、HTTPS-first scheme policy，以及 portable TimeProvider／typed API request builders。Time Source → TimeProvider → WF-RAC API Envelope → Transport；AirconStat Codec → Base64 → API Envelope。Transport 是最底層 opaque byte/text boundary，不依賴 Protocol、Device Model 或 Matter；其 request／response body 不知道 AirconStat、Base64、JSON schema、identity 或 timestamp semantics。HTTP status 是上層 application data，不等同 transport error。ESP-IDF adapter 已實作 bounded synchronous single-attempt POST 與 caller-owned response collection，但本 task 沒有執行任何 network I/O。

專案保持 monorepo，但 reusable portable library components 是 `wfrac_protocol`、`wfrac_device_model`、`wfrac_api`、`wfrac_transport`、`wfrac_client` 與 `wfrac_persistence`；ESP-IDF platform adapters 包含 `wfrac_transport_espidf`、`wfrac_time_espidf` 與 `wfrac_persistence_espidf`，分別依賴 portable transport／HTTP diagnostics、portable API 與 typed persistence contract。`firmware/idf_compile_smoke` 只是 reference build consumer，不是 portable component 的隱性 dependency。library packaging、versioning 與外部 distribution 尚未開始。

Typed persistence只保存 bounded Operator ID、confirmed scheme、historical time checkpoint；`wfrac_persistence_espidf → wfrac_persistence → wfrac_protocol（CRC）/wfrac_transport（SchemeKnowledge）`，沒有ESP-IDF types進入portable API。Fake store只供host tests，NVS adapter只source／compile/link；future runtime明確initialize、serialize、load/adopt，existing identity／scheme／time owner不被store取代，不自動修改ApiClientContext、SchemeCache、RuntimeOrchestrator或system clock。Schema／錯誤／key policy見 [persistence boundary](references/topics/persistence-boundary.md)；secret-at-rest、actual durability／power-loss／hardware evidence仍pending。

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

`wfrac_client` 的 request executor 不 serialize JSON、不建 timestamp、不 parse response，也不實作 HTTP/TLS。它將已建立的 `ApiRequestView` 以原 body 交給首次 attempt，立即以 SchemePolicy 記錄結果；只有 policy 明確回傳 `retry_http_compatibility` 才以同一 body 作唯一 HTTP fallback。因此每次 executor 最多兩次 physical attempts，沒有 loop 或 automatic rediscovery。獨立的 portable post-write verifier 使用既有 API builder／executor／response parser 與 Device Model reconciliation：`setAirconStat` accepted 後只進入 verifying，caller 以 bounded policy 排程每一次 `getAirconStat` readback；matching authoritative observation 才 confirmed，mismatch 保留 observed state，且不重送 write。它不假定 settle delay、poll interval、writer identity 或 physical actuation 已被確認。

raw HTTP response bytes → `wfrac_api` response envelope → command/result/contents → typed `getDeviceInfo` bootstrap fields。response parser 不依賴 HTTP status、Transport 或 ESP-IDF；nonzero result 只是保留的 command-specific data，不是 generic parse failure。`getAirconStat` contents parser 以 caller-owned scratch 將 JSON-decoded Base64 text 交給 `wfrac_protocol::base64_decode`，再以 `parse_aircon_stat` 驗證結構與 COMMAND／RECEIVE CRC；只有兩個 CRC 都有效才輸出 `AirconFrameView`，供未來 Protocol semantic／Device Model integration 使用。

```text
HTTP response bytes
        ↓
API envelope → getAirconStat contents → Base64 → raw AirconStat
        ↓
parse_aircon_stat → COMMAND + RECEIVE dual CRC
        ↓
validated AirconFrameView
        ↓
RECEIVE semantic decode → BasicHvacState → DeviceState observation/reconciliation
```

`wfrac_device_model` 的 read-path boundary 只認 `AirconFrameView`，不依賴 API、Transport 或 ESP-IDF，並在 mapping 時再次確認雙 CRC 與基本 block layout。成功 observation 不改 availability；pending command 只在 `sent`、`accepted`、`verifying` lifecycle phase中等待外層 resolution。reconciliation 的 observed-converged 只表示 state target 與 observation 相同，並非 transport confirmation、writer identity 或 physical command causality。

```text
future trusted/estimated time source
        ↓ explicit seed/adopt
ESP-IDF POSIX system clock
        ↓ gettimeofday
EspSystemTimeProvider → ITimeProvider → WF-RAC API builder
```

`wfrac_time_espidf` 是 platform adapter，只依賴 portable `wfrac_api`。它不取得時間：無 SNTP、DNS、Internet、Wi-Fi、RTC 或 NVS；quality 在每個 provider instance 初始 unavailable。seed 的 `settimeofday()` 具 process/device-wide global clock effect，adopt 不改 clock，兩者都必須由 future runtime 明確呼叫。

`firmware/wfrac_bootstrap_probe` 是獨立的 diagnostic firmware consumer，不是 portable component、production Wi-Fi owner 或未來 Matter provisioning architecture。它的預設組態為 inert；source 只建立 RAM-only、bounded one-shot STA 與 read-only `getDeviceInfo` wiring。其 compile/link evidence 只能標示 `COMPILE-READY`，實際 Wi-Fi、TLS、WF-RAC 與 hardware validation 仍為 `HARDWARE_TEST_PENDING`。

另有 default-off diagnostic WebUI path：embedded form → bounded firmware-local POST parser → shared one-shot executor/result → existing TimeProvider/API/client/transport boundaries。Temporary WPA2 SoftAP 只在 explicit submit 時進 APSTA，一次 STA connect（20 秒 bound）／getDeviceInfo（5 秒 transport timeout），不 retry/poll/fallback；結束回 AP-only 供下一次 explicit 操作。Submitted STA/WF-RAC credentials／scratch 為 RAM-bound，UI 不反射敏感輸入／response identity；HTTPS trust 仍 explicit。AP passphrase 是本地 build input，會存在 local configuration／firmware image，必須保護其 artifacts；這不是 application credential persistence。Wi-Fi／HTTPD lifecycle 留在 firmware consumer，不新增 portable dependency／production provisioning architecture。

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

## Portable orchestration step boundary

`wfrac_client::RuntimeOrchestrator` 只由 caller 驅動 `step()`；每 step 最多一個 logical API operation，physical attempts 仍由既有 executor／SchemePolicy 決定。沒有 scheduler、thread/task owner 或 autonomous polling。Bounded queue（固定 storage 上限 4，caller 可縮小）coalesce 尚未 dispatch 的同欄位 intent；pending verification 優先於下一個 write／一般 poll。較新的 desired 不取消舊 pending，仍由既有 Device Model reconciliation 保護／清除。

Control raw bytes 由既有 observed-baseline builder 產生；每次 write 前重新 parse cached raw storage、確認雙 CRC 與 caller-defined baseline age，任何可能 delivered write 都使舊 baseline 失效。Readback 沿用 verifier，只在 mismatching observation／remaining policy budget 下排程下一步，transport failure 不復活 pending、不重送 write。普通 poll 僅對 timeout／connection／I/O failure 作 bounded read retry decision，TLS identity／TLS failure 不 downgrade 或 retry burst。

Caller 提供 monotonic clock、Unix TimeProvider 及所有 poll/settle/retry/deadline/age policy；不猜 physical timing。同步 operation 的 deadline 在前後檢查，不中斷既有 transport call；late observation 仍更新 authoritative observed state，但 report 為 timed_out。Clock regression fail closed，recovery 由 caller 負責。Availability 只由 authoritative read success／configured consecutive read-failure threshold 更新，不由 write acceptance 或 local time-unavailable 推定。所有 storage/strings 非 owning、externally serialized、無 explicit heap；portable dependency 不變，platform runtime integration 仍 pending。

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

`wfrac_matter_adapter` 是 SDK-free、software-only 的 project mapping boundary，dependency 僅 `Matter adapter → wfrac_device_model`，不成為 portable core 的反向 dependency。它以 available authoritative observed state 投影 On/Off、Off/Cool/Heat 與對應 setpoint，controller room input 作 explicit nullable temperature proxy；write 只形成單一 `CommandIntent`，交回既有 runtime admission／lifecycle，不自行 dispatch。Unknown／unsupported 不變成 fabricated attribute defaults，desired／pending 不作 optimistic observed truth。詳細 [mapping contract](references/topics/matter-rac-mapping.md)；完整 endpoint conformance、Matter SDK／commissioning／hardware 仍未開始。
