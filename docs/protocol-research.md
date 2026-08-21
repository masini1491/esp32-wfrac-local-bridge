# WF-RAC protocol research

本文件持久化已完成的 Phase 0.1 Reference Deep Review 結論；不代表本 project 已完成實機驗證或 runtime implementation。

## Evidence terminology

`CONFIRMED_UPSTREAM`：由 pinned reference 明確支持。`CONFIRMED_LOCAL`：必須有本 project hardware/log evidence。`INFERRED`：推論。`UNKNOWN`：尚未確定。`HARDWARE_TEST_PENDING`：等待使用者 WF-RAC 硬體驗證。

本文件的 upstream facts 均不是本 project hardware claims。`CONFIRMED_LOCAL` 目前為 **none**；使用者 WF-RAC protocol/runtime validation 為 `HARDWARE_TEST_PENDING`。

## API envelope — CONFIRMED_UPSTREAM

- TCP port：`51443`
- API path：`/beaver/command/<command>`
- JSON request envelope：`apiVer`、`command`、`deviceId`、`operatorId`、`timestamp`、optional `contents`
- Observed `apiVer`：`1.0`
- `timestamp`：Unix seconds
- Reviewed command family：`getDeviceInfo`、`getAirconStat`、`setAirconStat`、`updateAccountInfo`、`deleteAccountInfo`

以上尚未在本 project hardware 上確認。

## HTTP / HTTPS transport — CONFIRMED_UPSTREAM

Architecture 保留兩類 transport：legacy `HTTP :51443` 與較新 `WF-RAC-HTTPS` 的 `HTTPS :51443`。較新 current integration 觀察到 self-signed certificate、TLS 1.2、ALPN 限制為 `http/1.1`、keep-alive disabled、`Connection: close`；upstream comment 指出 device 一次只接受一個 connection。Request header casing 可能重要，current upstream implementation 刻意保留 casing；觀察 headers 包含：

```text
Content-Type: application/json; charset=utf-8
User-Agent: smartmair_app[1.4.005]
Accept: */*
Connection: close
```

這些是 observed upstream behavior，不宣稱所有 WF-RAC firmware 都需要完全相同條件。本 project hardware 仍為 `HARDWARE_TEST_PENDING`。

## Protocol detection — CONFIRMED_UPSTREAM / local pending

較新的 Homebridge behavior 為 HTTPS first，failure 後 HTTP fallback；對 HTTPS-only port 發 plain HTTP probe 可能等待 request timeout。較舊 Homey behavior 曾採 HTTP first、HTTPS fallback。因此本 project future architecture direction 為 HTTPS-first detection，但 local devices 仍為 `HARDWARE_TEST_PENDING`；本階段不實作 detection。

## Identity model — CONFIRMED_UPSTREAM

`MAC Address`、`Device ID`、`Aircon ID`、`Operator ID` 是不同概念，不得假設相同。某些 firmware 的 `getDeviceInfo` 回報 `airconId` 可能與 MAC 不同；`updateAccountInfo` 必須使用 device-reported `airconId`。Future Config / Device Model 必須保留獨立 `airconId` field。

Observed `getDeviceInfo` contents 包含：`airconId`、`macAddress`、`apMode`；其他 mapping 觀察到 `firmType`、MCU firmware、Wi-Fi firmware、`numOfAccount`、`timezone`。欄位是否存在於各 firmware：`HARDWARE_TEST_PENDING`。

## Operator ID and registration — CONFIRMED_UPSTREAM

Operator ID 代表 controller / remote identity。Current integrations 支援：

1. **Self-managed**：bridge 產生並持久化 Operator ID，自行 `updateAccountInfo`，移除時 `deleteAccountInfo`。
2. **Mirror**：使用既有 Smart M-Air Operator ID，不自行 register/deregister。

同一 Operator ID 可跨安裝中的多台 WF-RAC 共用。`updateAccountInfo.contents` 觀察到 `accountId`、`airconId`、`remote`、`timezone`，其中 observed `remote` 為 `0`。Upstream 顯示 remote/controller slot 上限為 4 accounts。

Result semantics 必須 command-specific：某些 command 的 `result = 2` 被視為 unregistered，而 `updateAccountInfo` 的 `result = 2` 被視為 account limit。不可建立全域 `result 2` 固定意義規則，並須預留 firmware variation。

## Discovery — CONFIRMED_UPSTREAM

Homey integration 使用 mDNS-SD；service name `beaver`、protocol `tcp`。這是 future local discovery research direction，本 project hardware 為 `HARDWARE_TEST_PENDING`，本階段不實作。

## Request serialization and timing — CONFIRMED_UPSTREAM

兩套 upstream implementations 均顯示 WF-RAC 不適合高頻平行 request：需要 serialized request/command queue、status polling 不與 active command 同時執行、approximately 1000 ms request pacing、bounded timeout/retry 與 connection-error backoff。較新 implementation 觀察：`MIN_REQUEST_GAP_MS = 1000`、request timeout `10 seconds`、`call()` default `3 retries`，以及約 `1s → 2s → 4s` 的 bounded exponential backoff。

這些是 code behavior，不是 hardware protocol requirement；initial attempt 與 bounded retries 的 exact count 不可全域化。Future architecture：`Matter callback → Device Model → bounded command queue → WF-RAC backend`，禁止 callback synchronous HTTP。

## AirconStat codec — CONFIRMED_UPSTREAM

`getAirconStat` / `setAirconStat` 使用 `contents.airconStat`；`airconStat` 是 Base64-encoded binary state/control payload，upstream codec 使用 CRC16-CCITT。

Observed state field family：`operation`、`presetTemp`、`operationMode`、`airFlow`、`windDirectionUD`、`windDirectionLR`、`entrust`、`coolHotJudge`、`modelNo`、`isVacantProperty`、`errorCode`、`indoorTemp`、`outdoorTemp`、`electric`。Operation modes 包含 `auto`、`cool`、`heat`、`fan`、`dry`；airflow 包含 `auto`、`lowest`、`low`、`high`、`highest`；另有 vertical/horizontal direction、3D Auto/entrust、vacant-property mode concepts。各 user device support：`HARDWARE_TEST_PENDING`。

`indoorTemp`、`outdoorTemp` 的 protocol presence 為 `CONFIRMED_UPSTREAM`，但 local reporting pending。`electric` 為 energy-related encoded field，protocol presence confirmed；單位、reset behavior 與 model reliability 仍 pending，不宣稱所有型號支援可靠電量統計。

## Read / modify / write model — CONFIRMED_UPSTREAM behavior

`setAirconStat` 不是單一 REST-style `setTemperature(25)`；future flow 應為：

```text
validated current state snapshot
→ apply requested delta
→ encode writable command state
→ serialized setAirconStat
→ parse returned airconStat
→ update Device Model
→ Matter update
```

Read-only/measured state（如 `electric`、`errorCode`、`indoorTemp`、`outdoorTemp`）不可當作普通 writable controls。不得讓每個 Matter attribute callback 各自發出 parallel request。

## Evidence matrix

### CONFIRMED_UPSTREAM

Port/path、JSON envelope、HTTP/HTTPS variants、TLS/HTTP observations、上述五個 command family、Operator ID、`airconId` distinct identity、registration、mDNS `beaver/tcp`、Base64 AirconStat、CRC16-CCITT、state field family、indoor/outdoor temperature presence、energy-related field、serialized request、約 1 秒 pacing、以及 esp-matter Room Air Conditioner。

### CONFIRMED_LOCAL

None.

### UNKNOWN / future mapping

Exact transport variant per device、optional fields per model、final Matter mapping for non-core functions、optimal poll interval、exact retry policy、energy semantics across models，以及 Google Home / Home Assistant controller UI behavior。
