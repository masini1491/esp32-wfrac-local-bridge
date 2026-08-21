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

## Targeted reference update — CONFIRMED_UPSTREAM

本節記錄 pinned `blues-sechseck/Mitsubishi-WF-RAC-Integration`（commit `928efd8637bcb346a4945c545ab76733eae6245c`、MIT）的 targeted review。其 `[HW]`、`[FW]`、`[APP]`、`[EXT]`、`[INF]` 標記全部仍是外部 reference evidence；本 project 的 `CONFIRMED_LOCAL` 仍為 none。

### Module architecture 與 provenance

上游 firmware analysis 顯示 WF-RAC module 為雙處理器架構：`Realtek RTL8710C → UART → Renesas RL78 → CNS / SPI → MHI AC main board`。Realtek side 負責 Wi-Fi、HTTP／HTTPS、MQTT、TLS、Base64、OTA 與 network plumbing；RL78 side 負責 CNS／SPI protocol bridge、AirconStat 與 MHI CNS/SPI frame 的轉換，以及 remote-control role。

因此 AirconStat 可理解為 `JSON／Base64 transport representation → AirconStat → CNS/SPI-derived state／control representation`。這是 protocol semantics 的 provenance view；本 project 仍只透過 WF-RAC network module 控制，不加入直接操作 CNS/SPI 的 runtime layer。

### mDNS discovery

觀察到的 mDNS-SD service 為 `_beaver._tcp`；instance name 是不含冒號且使用 lowercase 的 MAC address，host 為 `<mac>.local`，TXT 為 empty，expected API port 為 `51443`。Discovery result 只能作提示；`51443` 是 expected API default，不應把 mDNS advertised port 當作絕對 protocol truth，因為上游曾處理 device 錯誤 advertise 非 `51443` port 的情況。本階段不實作 discovery。

### Local API command inventory

上游 firmware analysis 觀察到 local handlers：`getDeviceInfo`、`getAirconStat`、`setAirconStat`、`updateAccountInfo`、`deleteAccountInfo`、`setNetworkInfo`、`updateFirmware`。本 project 初期只關心前五項；`setNetworkInfo` 僅記為 provisioning-related endpoint，`updateFirmware` 僅記錄存在且不規劃由本 project 自行驅動。`setTopicId`、`putAirconStat`、`reportAirconStat` 屬於 module cloud/MQTT side，不是 local HTTP handlers。

### Request envelope 與 module clock

Mandatory request fields 為 `command`、`apiVer`、`operatorId`、`deviceId`、`timestamp`；`contents` 依 handler 驗證，`getDeviceInfo` 可省略。`command`、`apiVer`、`operatorId`、`deviceId` 必須存在且非空；`timestamp` 必須存在且 non-zero。`apiVer = "1.0"` 是 observed normal value，但 upstream analysis 沒有顯示 firmware 會把它絕對比較為只能接受 `1.0`。

Request `timestamp` 不只是 freshness field：WF-RAC module 會把收到的 timestamp 寫入自身 time global，且 module 沒有 RTC。因此 request 可成為 module time source，可能影響 account expiry、timezone-related behavior、MQTT `aggTm` 與其他 time-derived metadata。未來 bridge 必須研究可靠 Unix time source；在無 Internet、ESP32 reboot 或尚未取得可信 wall-clock time 時如何安全處理 timestamp，目前為 `UNKNOWN / architecture research pending`，不得假設 Internet NTP 是生命線，也不實作 RTC/NTP。

### getDeviceInfo bootstrap 與 registration

`getDeviceInfo` 不需要先知道 `airconId`，`contents` 可省略，並可回傳 `airconId`、`macAddress`、`apMode`。Future onboarding direction 為：known host/IP → `getDeviceInfo` → discover actual `airconId` → persist identity/config → optional registration → normal status/control。`MAC`、`deviceId`、`airconId`、`operatorId` 仍是不同 identity。

`getAirconStat` 可在 operator 尚未註冊時讀取；`setAirconStat` 會驗證 `operatorId`。正式 persistent client 應使用 `updateAccountInfo` 註冊自己的 Operator ID；account table 為 4 slots，`deleteAccountInfo` 可釋放 slot。上游曾觀察到某些 firmware 可能接受 `operatorId = "aws"` 作為特殊 write escape hatch；這只記為 `CONFIRMED_UPSTREAM` diagnostic behavior，不是正式 architecture、default Operator ID 或 production control 依賴，也不實作。

### Result 與 transport details

上游 code table 可見 `0`、`1`、`2`、`10`、`11`、`12`、`20`、`99`、`429`，但只作參考。`result` 仍必須 command-specific parsing，並容忍 firmware variation；`result = 2` 可在不同 context 表示 account table full、registration expired 或 unregistered/write refusal，不是 universal definition。

Transport 使用 `POST` 到 `/beaver/command/<commandName>`，port 為 `51443`；older WF-RAC 使用 HTTP，WF-RAC-HTTPS／WCBN4612L 使用 HTTPS。部分 module 即使 body 是有效 JSON，response `Content-Type` 也可能是 `text/plain`；error HTTP status 也可能帶有有用 JSON body。因此 future parser 必須分開解析 transport status 與 protocol body，不能只因 response Content-Type 不是 `application/json` 就放棄 JSON parse。本階段不實作。

TLS compatibility strategy 依 firmware branch 與 reference 而異；上游 implementation 可能停用 hostname/certificate verification、採 older TLS compatibility 或降低 OpenSSL security level，也支援使用 user-provided certificate file 建立較可信 SSL context。這些都是 compatibility strategy，不是 protocol requirement；不得把 TLSv1 minimum、`SECLEVEL=1` 或 verify off 寫成 universal rule。Production 永久停用 TLS verification 前必須有文件化 security decision。

### Request timing 與 concurrency

上游 analysis 包含 `429 Too Many Requests`、`Connection rate exceeded`、device lock timeout 與 lock/semaphore behavior。實務原則為 one connection at a time，加上至少約 1 秒 request gap。未來架構必須使用 serialized request queue、bounded queue、每台 device 一個 active transaction、minimum request gap，以及 polling/command coordination；不得讓 status poll、set command 與 diagnostic operation-data request 對同一台 WF-RAC 平行發送。不同 integration 的 timeout 可能不同，因此 timeout duration 不是 protocol constant；只固定 bounded timeout，exact value 為 `HARDWARE_TEST_PENDING / implementation tuning`。

### AirconStat binary structure 與 CRC

Base64 decode 後的 AirconStat 包含兩個獨立區塊：`COMMAND block` 與 `RECEIVE block`。每個 block 由 18-byte state、1-byte segment count、N 個 4-byte variable segments、2-byte CRC 組成。COMMAND block 是希望 AC 執行的 control representation；RECEIVE block 是 AC 回報的 state representation。COMMAND block 為 variable-length，因此 RECEIVE start 必須由 COMMAND segment count 計算，不可假設固定 offset。

CRC16-CCITT exact parameters：polynomial `0x1021`、init `0xFFFF`、reflect input `no`、reflect output `no`、final XOR `none`、stored byte order `little-endian`。每個 block 分別涵蓋 state、segment count 與 segments，並各自擁有 CRC。本階段不加入 CRC source code。

Official app／firmware 顯示 variable segment default trailer 為 count `1` 加上 `FF FF FF FF`，概念為 `[0x01, 0xFF, 0xFF, 0xFF, 0xFF]`；code `0xFF` 是 no request／nothing to send sentinel。本階段不建立 encoder。

### CNS/SPI state mapping 與 set-bit semantics

18-byte AirconStat state block 與 MHI CNS/SPI frame 有明確對應：`state[2]`／DB0 為 power、mode、vertical swing；`state[3]`／DB1 為 fan、vertical vane position；`state[4]`／DB2 為 target temperature；`state[5]`／DB3 為 room temperature input/current controller temperature；`state[6]`／DB4 為 error code；`state[8]`／DB6 為 operation-data／cool-hot judge；`state[9]`／DB7 為 per-indoor compressor demand bit；`state[10]` 為 vacant／occupancy-related state；`state[11]` 為 horizontal vane；`state[12]` 為 3D Auto／Entrust-related flags。

MHI CNS/SPI command representation 不只是寫入 value；許多 writable fields 必須同時設定 value bits 與 corresponding set-bit，包括 Power、Mode、Fan、Vane、Setpoint。未來 AirconStat Codec 必須區分 decode receive state 與 encode command delta；不可採用「讀取 state bytes → 任意修改 value → 原樣回傳」的簡化策略。

`command[5]`／DB3 可能允許向 AC 提供 external room temperature；official app 通常使用 `0xFF` 表示不注入。此 write path 屬 `INFERRED / research-only`，具有控制風險，不列為已支援功能，也不實作。

### Operation-data segments

Variable operation-data segment 為 4 bytes：`[Code, OP1, OP2, OP3]`。觀察到 OP1 semantics：`0x10` 為 status response、`0xFF` 為 request/query、`0x00` 為 write command。`OP1 = 0x00` 代表 generic write path，可能影響 AC main board，且許多 code 尚未完整理解；Phase 0 只研究 read/query semantics，禁止 generic operation-data write。

普通 status polling 可收到重要 unsolicited segments：`0x80`／selector `0x10` 為 outdoor temperature、`0x80`／selector `0x20` 為 indoor temperature、`0x94` 為 energy-related counter。三者 protocol presence 為 `CONFIRMED_UPSTREAM`，實際使用者 device 仍為 `HARDWARE_TEST_PENDING`。

Temperature 使用非線性 lookup mapping，且 indoor／outdoor 使用不同 table。只記錄此 behavior，不複製 exact lookup table 或 formulas；未來 implementation 需進行 provenance/license review 與 local validation。

`0x94 energy` 是 current-run counter，不是 lifetime total：以 0.25 kWh step 增加，running 時增加，off 時保留最後 value，下一次 power-on reset to 0。Multi-split evidence 顯示它是 per indoor unit，而非整個 outdoor unit 共用 counter。若未來需要 lifetime energy，必須由 bridge 自行 persistent accumulation；因 0.25 kWh granularity，每次 run 最後不足一個 step 的部分會遺失，因此 derived lifetime total 只能是 approximate，不是 precision energy meter。

Multi-split 中 `state[9]` bit `0x02` 是 per-indoor-unit compressor demand，不等於 outdoor compressor 是否實際旋轉；operation-data code `0x11` compressor frequency 則是 outdoor-unit-level value。Future Device Model 不得把兩者合併成單一 boolean。其他 operation-data code 包含 `0x90` operating current、`0x85` discharge／hot-gas temperature、`0x13` EEV-related value、`0x81` indoor coil temperature、`0x87` indoor coil outlet temperature、`0x82` outdoor coil related raw value；code presence 可標 `CONFIRMED_UPSTREAM`，exact physical conversion 若上游仍屬 `[INF]` 則保持 `INFERRED`。

較偏 outdoor-unit shared 的 observation 包含 compressor frequency、operating current、discharge temperature、outdoor temperature 與部分 outdoor coil data；較偏 per-indoor 的 observation 包含 compressor demand、EEV-related state、indoor coil temperature、indoor coil outlet temperature 與 current-run energy counter。這份分類不宣稱適用所有 MHI models，並保留 `CONFIRMED_UPSTREAM` 加 `HARDWARE_TEST_PENDING`。

官方 app 使用 variable operation-data code `248` 處理 Home Leave／auto-heating related settings，sub-code family 與 read/write mechanism 已有 upstream round-trip；本 project 不把 Home Leave 列為 Matter core requirement，mapping 為 `UNKNOWN / Phase 0.3 pending`。

## Evidence matrix

### CONFIRMED_UPSTREAM

Port/path、JSON envelope、HTTP/HTTPS variants、TLS/HTTP observations、上述五個 command family、Operator ID、`airconId` distinct identity、registration、mDNS `beaver/tcp`、Base64 AirconStat、CRC16-CCITT、state field family、indoor/outdoor temperature presence、energy-related field、serialized request、約 1 秒 pacing、esp-matter Room Air Conditioner、dual-MCU WF-RAC architecture、RL78 CNS/SPI bridge role、AirconStat ↔ CNS/SPI relationship、`_beaver._tcp`、七個 local handlers、timestamp sets module time、`getDeviceInfo` bootstrap、AirconStat COMMAND/RECEIVE dual block、variable-length block location、exact CRC16 parameters、no-request sentinel、state ↔ CNS/SPI mapping、write set-bit semantics、4-byte operation-data segment、unsolicited temperature segments、current-run energy behavior、multi-split per-indoor／outdoor-shared observations、one connection／約 1 秒 spacing、unexpected Content-Type 下仍可能有用的 JSON body，以及 transport status／protocol body separation。

### CONFIRMED_LOCAL

None.

### UNKNOWN／未來 mapping

各 device 的確切 transport variant、各 model 的 optional fields、非核心功能的最終 Matter mapping、最佳 poll interval、確切 retry policy、跨 model 的 energy semantics，以及 Google Home／Home Assistant controller UI 行為。
