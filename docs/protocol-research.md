# WF-RAC protocol research

本文件記錄已完成的 Phase 0.1 Reference Deep Review 結論；不代表本 project 已完成實機驗證或 runtime implementation。

## Evidence terminology

`CONFIRMED_UPSTREAM`：已由 pinned upstream reference 確認。`CONFIRMED_LOCAL`：已由本專案實機或 log 確認。`INFERRED`：推論。`UNKNOWN`：尚未確定。`HARDWARE_TEST_PENDING`：等待實機驗證。

本文件的上游事實均不是本 project hardware claims。`CONFIRMED_LOCAL` 目前為 **none**；使用者 WF-RAC protocol/runtime validation 為 `HARDWARE_TEST_PENDING`。

## API 封裝層（API Envelope）— CONFIRMED_UPSTREAM

- TCP port：`51443`
- API path：`/beaver/command/<command>`
- JSON request envelope：`apiVer`、`command`、`deviceId`、`operatorId`、`timestamp`、optional `contents`
- 觀察到的 `apiVer`：`1.0`
- `timestamp`：Unix seconds
- Reviewed command family：`getDeviceInfo`、`getAirconStat`、`setAirconStat`、`updateAccountInfo`、`deleteAccountInfo`

以上尚未在本 project hardware 上確認。

## HTTP／HTTPS 傳輸層（Transport）— CONFIRMED_UPSTREAM

架構保留兩類 transport：legacy `HTTP :51443` 與較新 `WF-RAC-HTTPS` 的 `HTTPS :51443`。目前上游整合觀察到 self-signed certificate、TLS 1.2、ALPN 限制為 `http/1.1`、停用 keep-alive、`Connection: close`；上游註解指出 device 一次只接受一個 connection。Request header casing 可能重要，目前上游 implementation 刻意保留 casing；觀察到的 headers 包含：

```text
Content-Type: application/json; charset=utf-8
User-Agent: smartmair_app[1.4.005]
Accept: */*
Connection: close
```

這是上游實作行為，不代表所有 WF-RAC firmware 的 protocol 要求。本 project hardware 仍為 `HARDWARE_TEST_PENDING`。

## Protocol detection — CONFIRMED_UPSTREAM／本地待驗證

較新的 Homebridge 行為為先嘗試 HTTPS，failure 後 fallback 至 HTTP；對 HTTPS-only port 發送 plain HTTP probe 可能等待 request timeout。較舊 Homey 行為曾採先 HTTP、再 fallback 至 HTTPS。因此本 project 未來架構方向為 HTTPS-first detection，但 local devices 仍為 `HARDWARE_TEST_PENDING`；本階段不實作 detection。

## Identity model — CONFIRMED_UPSTREAM

`MAC Address`、`Device ID`、`Aircon ID`、`Operator ID` 是不同概念，不得假設相同。某些 firmware 的 `getDeviceInfo` 回報 `airconId` 可能與 MAC 不同；`updateAccountInfo` 必須使用 device-reported `airconId`。未來 Config／Device Model 必須保留獨立的 `airconId` field。

觀察到的 `getDeviceInfo` contents 包含：`airconId`、`macAddress`、`apMode`；其他 mapping 觀察到 `firmType`、MCU firmware、Wi-Fi firmware、`numOfAccount`、`timezone`。欄位是否存在於各 firmware：`HARDWARE_TEST_PENDING`。

## Operator ID and registration — CONFIRMED_UPSTREAM

Operator ID 代表 controller／remote identity。目前上游整合支援：

1. **Self-managed**：bridge 產生並持久化 Operator ID，自行 `updateAccountInfo`，移除時 `deleteAccountInfo`。
2. **Mirror**：使用既有 Smart M-Air Operator ID，不自行 register/deregister。

同一 Operator ID 可由安裝中的多台 WF-RAC 共用。`updateAccountInfo.contents` 觀察到 `accountId`、`airconId`、`remote`、`timezone`，其中 `remote` 的觀察值為 `0`。上游顯示 remote/controller slot 上限為 4 accounts。

Result semantics 必須 command-specific：某些 command 的 `result = 2` 被視為 unregistered，而 `updateAccountInfo` 的 `result = 2` 被視為 account limit。不可建立全域 `result 2` 固定意義規則，並須預留 firmware variation。

## Discovery — CONFIRMED_UPSTREAM

Homey integration 使用 mDNS-SD；service name `beaver`、protocol `tcp`。這是未來本地探索的研究方向，本 project hardware 為 `HARDWARE_TEST_PENDING`，本階段不實作。

## Request serialization and timing — CONFIRMED_UPSTREAM

兩套上游 implementation 均顯示 WF-RAC 不適合高頻平行 request：需要 serialized request／command queue、status polling 不與 active command 同時執行、約 1000 ms request pacing、bounded timeout/retry 與 connection-error backoff。較新的 implementation 觀察到：`MIN_REQUEST_GAP_MS = 1000`、request timeout `10 seconds`、`call()` default `3 retries`，以及約 `1s → 2s → 4s` 的 bounded exponential backoff。

這些是 code 行為，不是 hardware protocol requirement；initial attempt 與 bounded retries 的 exact count 不可全域化。未來架構應為：`Matter callback → Device Model → bounded command queue → WF-RAC backend`，禁止 callback synchronous HTTP。

## AirconStat 編解碼器（Codec）— CONFIRMED_UPSTREAM

`getAirconStat` / `setAirconStat` 使用 `contents.airconStat`；`airconStat` 是 Base64-encoded binary state/control payload，upstream codec 使用 CRC16-CCITT。

觀察到的 state field family：`operation`、`presetTemp`、`operationMode`、`airFlow`、`windDirectionUD`、`windDirectionLR`、`entrust`、`coolHotJudge`、`modelNo`、`isVacantProperty`、`errorCode`、`indoorTemp`、`outdoorTemp`、`electric`。Operation modes 包含 `auto`、`cool`、`heat`、`fan`、`dry`；airflow 包含 `auto`、`lowest`、`low`、`high`、`highest`；另有 vertical/horizontal direction、3D Auto/entrust、vacant-property mode 等功能概念。各 user device 的支援情況：`HARDWARE_TEST_PENDING`。

`indoorTemp`、`outdoorTemp` 的 protocol presence 為 `CONFIRMED_UPSTREAM`，但本地回報仍待驗證。`electric` 為 energy-related encoded field，protocol presence 已確認；單位、reset behavior 與各型號可靠度仍待驗證，不宣稱所有型號支援可靠電量統計。

## 讀取／修改／寫入 state model — CONFIRMED_UPSTREAM 行為

`setAirconStat` 不是單一 REST-style `setTemperature(25)`；未來流程應為：

```text
validated current state snapshot
→ apply requested delta
→ encode writable command state
→ serialized setAirconStat
→ parse returned airconStat
→ update Device Model
→ Matter update
```

Read-only/measured state（如 `electric`、`errorCode`、`indoorTemp`、`outdoorTemp`）不可當作普通 writable controls。不得讓每個 Matter attribute callback 各自發出平行 request。

## Evidence matrix

### CONFIRMED_UPSTREAM

Port/path、JSON envelope、HTTP/HTTPS variants、TLS/HTTP observations、上述五個 command family、Operator ID、`airconId` distinct identity、registration、mDNS `beaver/tcp`、Base64 AirconStat、CRC16-CCITT、state field family、indoor/outdoor temperature presence、energy-related field、serialized request、約 1 秒 pacing、以及 esp-matter Room Air Conditioner。

### CONFIRMED_LOCAL

None.

### UNKNOWN／未來 mapping

各 device 的確切 transport variant、各 model 的 optional fields、非核心功能的最終 Matter mapping、最佳 poll interval、確切 retry policy、跨 model 的 energy semantics，以及 Google Home／Home Assistant controller UI 行為。
