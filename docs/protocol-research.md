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

R1 command-specific context matrix：`getDeviceInfo` 的 `result = 2` 目前為 `UNKNOWN`；`getAirconStat` 在較新 firmware 缺少 required `airconId` 時可能出現 HTTP 400／`result = 2`；`setAirconStat` 可在 expired／unregistered operator 或 write-refusal context 出現；`updateAccountInfo` 的 `result = 2` 可表示 account table full；`deleteAccountInfo` 的 `result = 2` 目前為 `UNKNOWN`。共同解讀必須同時保留 HTTP status、command、WF-RAC result 與 response body，不得使用 global `switch(result)` 定義所有 command semantics。Observed code family `0, 1, 2, 10, 11, 12, 20, 99, 429` 只作 upstream reference，不是所有 command 的 universal definition。

### R1 identity and registration boundaries

R1 targeted research 進一步確認：`deviceId` 是 request envelope 中的 client-side identifier。上游只確認它必須是存在、非空的 string，會被記錄並傳入 handler；目前沒有證據要求它等於 MAC、`airconId`、固定 UUID，或被作為 AC identity 驗證。因此本 project 將其定義為 stable、non-empty、opaque client identifier，未來方向為 bridge-scoped persistent identifier；這是 project recommendation，不是 universal protocol requirement。

`getDeviceInfo` 不需要 `contents`，也不需要事先知道 `airconId`；它回傳至少 `airconId`、`macAddress`、`apMode`，是 canonical bootstrap mechanism。未來概念流程為：known host/IP → persistent client `deviceId` → persistent `operatorId` → valid timestamp → `getDeviceInfo` → device-reported `airconId` → persist identity → optional/self-managed registration → normal status/control。不得假設 `airconId == MAC`；MAC 只能作 discovery/local identification hint，canonical `airconId` 以 device-reported value 為準。

Operator ID 是 controller／remote identity，必須是 stable opaque identifier；upstream 不同 implementation 的 UUID-based 或 prefixed generation strategy 只是 implementation strategy，不代表 RFC UUID 是 protocol requirement。`accountId` 不建立為第五個 identity；三個主要 upstream integrations 都以 `accountId = operatorId` 用於 `updateAccountInfo` 與 `deleteAccountInfo`，此關係標為 `STRONG_CORROBORATION`。

Self-managed registration 的概念順序為 persistent `operatorId` → bootstrap actual `airconId` → `updateAccountInfo`（`accountId = operatorId`、`airconId = device-reported airconId`、observed `remote = 0`、`timezone = local timezone value`）→ normal write。Mirror mode 使用既有 Operator ID，不自行 register/deregister。正常 reboot 不應自動 deregister；deregistration 應限於明確 ownership removal 或 reset policy。這是 architecture recommendation，不是 protocol constant。

Account table observed limit 為 4。`remoteList` 不可視為可靠的 registered-ID enumeration；上游曾觀察 `numOfAccount = 2` 但 `remoteList = ["", "", "", ""]`。Future bridge 只能可靠管理自己的 Operator ID；table full 時不得猜測或自動刪除未知 account，應要求 explicit user/operator action。`operatorId = "aws"` 仍只作 upstream diagnostic observation，不是 project architecture、default 或 production dependency。

R1 remaining unknowns：`deviceId` 是否存在 hidden handler-specific semantics、`remote = 0 / 1` 的完整語意、`timezone` 的真正 runtime effect、`expires` 的 exact calculation／duration／refresh semantics、各 firmware 完整 command × result matrix，以及 registration eviction／expiry 後跨 firmware 的 recovery behavior。以上維持 `UNKNOWN`；`CONFIRMED_LOCAL` 仍為 none。

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

## MHI CNS/SPI cross-reference — CONFIRMED_UPSTREAM

本節記錄 pinned `hberntsen/mhi-ac-ctrl-esp32`（commit `cb00cd54525acea9fbafdfee178a657eb6667faa`、MIT）的 targeted cross-reference。其 source facts 只能作 `CONFIRMED_UPSTREAM`；作者標示為 rough approximation、needs confirmation 或 community assumption 的內容，維持 `INFERRED` 或 `UNKNOWN`。本 project `CONFIRMED_LOCAL` 仍為 none。

### Frame 基礎與 AirconStat 對照

該 reference 定義 `MHI_FRAME_LEN_SHORT = 20`、`MHI_FRAME_LEN_LONG = 33`，並使用 SB0、SB1、SB2 與 DB0、DB1、DB2、DB3、DB4、DB6、DB9–DB15...DB26。高價值 semantic mapping 如下：

| CNS/SPI field | 主要語意 |
|---|---|
| DB0 | power、mode、vertical swing |
| DB1 | fan、vertical vane |
| DB2 | target temperature |
| DB3 | room／external temperature |
| DB4 | error |
| DB6 | operation-data selector／extended fan state |
| DB9–DB12 | operation-data channel |
| DB13 | direct CNS/SPI compressor state／heat-cool state |
| DB16 | horizontal vane |
| DB17 | horizontal swing／3D Auto |

與 `Mitsubishi-WF-RAC-Integration` 交叉比對後，高信心對照為：AirconStat `state[2] ↔ DB0`、`state[3] ↔ DB1`、`state[4] ↔ DB2`、`state[5] ↔ DB3`、`state[6] ↔ DB4`、`state[11] ↔ DB16`、`state[12] ↔ DB17`。這是 cross-reference，不表示 AirconStat 是完整 CNS frame 的 raw copy。

### Set-bit 與基本編碼

兩個 independent upstream paths 對 set-bit architecture 形成 `STRONG_CORROBORATION`：Power value 為 DB0 bit 0、set-bit 為 DB0 bit 1；Mode value 為 DB0 mode bits、set-bit 為 DB0 bit 5；Fan value 為 DB1 fan bits、set-bit 為 DB1 bit 3；target temperature value 為 DB2 bits 6:0、set-bit 為 DB2 bit 7；vertical swing value 為 DB0 bit 6、set-bit 為 DB0 bit 7；vertical vane position value 為 DB1 position bits、set-bit 為 DB1 bit 7。DB16／DB17 的 horizontal vane／swing 亦有 explicit set semantics。

AirconStat Codec 未來必須分開 decode RECEIVE state 與 encode COMMAND delta，不可只讀取 state、修改 value bits 後原封不動送回。CNS/SPI-side raw mode values 為 `auto=0x00`、`dry=0x04`、`cool=0x08`、`fan=0x0C`、`heat=0x10`；fan values 為 speed 1=`0`、speed 2=`1`、speed 3=`2`、speed 4=`6`、auto=`7`，並須注意 DB6 extended fan bit。這些是 CNS/SPI layer encoding，不可與 WF-RAC AirconStat layer 的數值無條件混成單一表格。

基本溫度 cross-reference：target temperature 為 `DB2 raw / 2`、resolution `0.5 °C`；room temperature 為 `(DB3 - 61) / 4`、resolution `0.25 °C`。這條 CNS/SPI basic path 與 operation-data 的高解析 indoor/outdoor temperature path 不可混為一談。

### Operation-data address space

`DB9`–`DB12` 明確作 operation-data request/response。交叉比對的 code 包含：`0x05` Set Temperature、`0x80` Air Temperature、`0x81` Indoor coil family、`0x87` Indoor suction/header temperature、`0x1F` Fan Speed、`0x1E` Run Hours、`0x82` Outdoor Heat Exchanger、`0x11` Compressor Frequency、`0x85` Discharge Pipe Temperature、`0x90` Current、`0xB1` Compressor Discharge Superheat、`0x7C` Compressor Protection State、`0x0C` Defrosting、`0x13` Outdoor Expansion Valve、`0x94` Energy。

這與 WF-RAC HTTP → AirconStat variable segments 觀察到的 address space 高度重疊，但 direct CNS/SPI capability 不等於 WF-RAC network-visible capability；不得因 direct CNS project 能讀某 code，就宣稱 WF-RAC HTTP 一定能讀。

`0x11` Compressor Frequency、`0x90` Current、`0x85` Discharge Pipe Temperature、`0x94` Energy 是 `STRONG_CORROBORATION`。`0x90` 的 upstream observed conversion 為 `raw × 14 / 51`；`0x94` 為 `raw = DB12 << 8 | DB11`、conversion `raw / 4`，即 0.25 kWh/count。後者與 WF-RAC Integration 的 0.25 kWh resolution 一致，但 current-run、power-on reset、off 保留與 multi-split per-indoor lifecycle semantics 主要仍來自 WF-RAC Integration evidence，不宣稱由 direct CNS reference 獨立驗證。

`0x13` 兩份 reference 都指向 EEV／expansion-valve data，code semantics 為 `CONFIRMED_UPSTREAM`，但屬 `PARTIAL_CORROBORATION`：direct CNS 使用 `DB12 << 8 | DB11` 的 16-bit pulse-related value，WF-RAC network representation 可能較窄，不得宣稱 HTTP `0x13` 等於 raw 16-bit CNS pulse count。

`0x80` outdoor temperature code semantics 為 `CONFIRMED_UPSTREAM`，但 exact conversion 是 `EVIDENCE_CONFLICT`：direct CNS reference 使用 `(raw - 94) × 0.25 °C`，WF-RAC Integration 依 Smart M-Air app／WF-RAC hardware evidence 使用非線性 lookup mapping。兩者均保留；WF-RAC network applicability 優先研究 app-derived mapping，最終仍為 `HARDWARE_TEST_PENDING`。部分 coil conversion（例如 `raw × 0.327 - 11.4`）由 source 自己標為 rough approximation 或 needs confirmation，故保持 `INFERRED / APPROXIMATE`，不複製公式。

### Compressor state boundary

必須分離三個概念：A. direct CNS/SPI `DB13` compressor running/idle 或 heat-cool state；B. WF-RAC AirconStat `state[9] bit 0x02` 的 per-indoor compressor demand；C. operation-data `0x11` 的 outdoor-unit-level compressor frequency。Future Device Model 不得把三者合併成 `compressorOn`；multi-split 中 indoor demand 可為 false、sibling demand 為 true，而 outdoor frequency 仍大於 0。Direct CNS `DB13` 只作 semantics cross-reference，不假設 WF-RAC AirconStat 一定暴露。

`0x7C` Compressor Protection State Number 為 `PARTIAL / UNRESOLVED`：address semantics 為 `CONFIRMED_UPSTREAM`，但 WF-RAC-visible operational meaning 為 `UNKNOWN / HARDWARE_TEST_PENDING`，不得直接暴露成 Compressor Protection Active。`0x0C` Defrosting 同樣是 direct CNS semantic `CONFIRMED_UPSTREAM`，但 WF-RAC HTTP availability 為 `UNKNOWN / HARDWARE_TEST_PENDING`。

DB3 external room temperature cross-reference：`DB3 = 0xFF` 表示 disable external input；有值時為 `round(T × 4 + 61)`。這與 WF-RAC `command[5] / DB3` 可能注入 external temperature 的研究吻合，但 WF-RAC HTTP write safety/effect 為 `INFERRED / HARDWARE_TEST_PENDING`，維持 research-only，不實作。

### Cross-reference matrix

| Topic | Mitsubishi-WF-RAC-Integration | mhi-ac-ctrl-esp32 | Assessment |
|---|---|---|---|
| Power／mode DB0 | AirconStat `state[2]` | DB0 value／set-bit | STRONG_CORROBORATION |
| Fan／vane DB1 | AirconStat `state[3]` | DB1 value／set-bit | STRONG_CORROBORATION |
| Setpoint DB2 | AirconStat `state[4]` | DB2 value／set-bit | STRONG_CORROBORATION |
| External room temp DB3 | `command[5]`／DB3 path | DB3 disable／encoding | STRONG_CORROBORATION；write safety仍待驗證 |
| Horizontal vane DB16 | AirconStat `state[11]` | DB16 | STRONG_CORROBORATION |
| 3D Auto DB17 | AirconStat `state[12]` | DB17 | STRONG_CORROBORATION |
| Set-bit semantics | COMMAND block | CNS/SPI value＋set-bit | STRONG_CORROBORATION |
| `0x11` compressor frequency | operation-data | operation-data | STRONG_CORROBORATION |
| `0x90` current | operation-data | operation-data | STRONG_CORROBORATION |
| `0x85` discharge temperature | operation-data | operation-data | STRONG_CORROBORATION |
| `0x94` energy | 0.25 kWh resolution | DB12/DB11、0.25 kWh/count | STRONG_CORROBORATION |
| `0x13` EEV | narrower WF-RAC representation 可能存在 | 16-bit CNS value | PARTIAL_CORROBORATION |
| `0x80` outdoor temperature conversion | nonlinear lookup | `(raw - 94) × 0.25` | EVIDENCE_CONFLICT |
| `0x81`／`0x87` coil conversion | operation-data presence | sensor family／rough formula | PARTIAL_CORROBORATION |
| `0x7C` protection | response/value常為0 | protection state number | PARTIAL / UNRESOLVED |
| `0x0C` defrost | 尚無充分 WF-RAC path evidence | Defrosting | DIRECT_CNS_ONLY |
| compressor state concepts | demand／frequency | DB13 direct state | STRONG_CORROBORATION；概念不可合併 |

此 matrix 是 semantics cross-reference，不是把 direct CNS/SPI capability 提升為 WF-RAC HTTP capability，也不是 implementation specification。

## R2：固定 18-byte AirconStat state／command semantics — CONFIRMED_UPSTREAM

R2 將 AirconStat 明確記錄為兩個獨立區塊：`COMMAND` 與 `RECEIVE`。每個區塊包含 18-byte state、1-byte segment count、N 個 4-byte variable segments 與 2-byte CRC16；`RECEIVE` 起點必須由 `COMMAND` 的 segment count 推導。RECEIVE raw state 不等於 COMMAND raw state，network mask 亦會遮蔽 command set bits，因此不能由 RECEIVE 重建 COMMAND。未實作 codec；未來邊界為 `decode_receive_state()` 與 `encode_command()`。

固定 state map：byte 0 是 model number 低 7 bits；byte 1 的 RECEIVE `0x04` 功能 UNKNOWN，COMMAND 安全預設 0；byte 2 是 power／mode／vertical swing，byte 3 是 fan／vertical vane，byte 4 是 target temperature（RECEIVE raw／2 °C、COMMAND temperature×2 並含 set bit `0x80`），byte 5 是 controller room temperature／external injection，COMMAND 安全預設 `0xFF`（no injection），且不得複製 RECEIVE；byte 6 是 read-only error；byte 7 的 `0x10` 會 echo command byte 8，不是獨立 compressor state；byte 8 是 operation-data／CoolHotJudge；byte 9 的 `0x02` 是 per-indoor compressor demand；byte 10 是 vacant／occupancy-related；byte 11、12 是 horizontal vane／swing／Entrust／3D Auto；byte 13、14、16、17 UNKNOWN，COMMAND 一律安全預設 0 且不得複製 RECEIVE；byte 15 的 self-clean bit 不可靠，不能當正常 status 或 command。所有 local support 仍為 `HARDWARE_TEST_PENDING`。

Network set-bit mask（18 bytes）為 `00 00 A2 88 80 00 00 00 00 00 00 10 0A 00 00 00 00 00`。COMMAND 的 value bits 與 set bits 必須依 intended semantics 產生。RECEIVE mode：auto `0x00`、cool `0x08`、heat `0x10`、fan `0x0C`、dry `0x04`；COMMAND mode 分別為 `0x20`、`0x28`、`0x30`、`0x2C`、`0x24`。RECEIVE fan 為 auto `0x07`、speed1–4 `0x00/0x01/0x02/0x06`；COMMAND 為 auto `0x0F`、speed1–4 `0x08/0x09/0x0A/0x0E`。reference prose 的 mode table 與實作對照不一致，保留為 `INTERNAL_REFERENCE_CONFLICT` 文件註記，不混入 evidence vocabulary。

Safe no-op/query 可清除 writable set bits、令 command byte 5 為 `0xFF`，必要時 carry-through byte 8；command block 不必是 full write。`setAirconStat` 的 selective delta 或 full re-encode 仍待 firmware validation，current-state re-encode 只是未來 implementation strategy，不是 universal protocol requirement。Self-clean 尚未建立可靠的 remote start、status 或 initial Device Model control。Direct CNS/SPI DB13、AirconStat byte 9、以及 operation-data `0x11` outdoor compressor frequency 必須保持分離，不得合併為 `compressorOn`。

R2 尚未確認：byte 1、13、14、16、17 的完整意義、CoolHotJudge／byte 8 完整 semantics、self-clean 差異、external temperature write safety、不同 firmware 的 selective/full write 策略、model-specific fields，以及所有 local hardware behavior。

## R3：Operation-data WF-RAC-visible matrix — CONFIRMED_UPSTREAM

R3 將 variable segment 固定記錄為完整 `[Code, OP1, OP2, OP3]` raw bytes；generic parser 必須先保存四者，再交由 code-specific decoder 解釋。`OP1=0xFF` 通常是 query/request、`OP1=0x00` 通常是 command path，但 response-side 不得建立 universal `OP1=0x10` 規則。

Visibility classification 與 evidence level 分離。`operation_data_r3` matrix 記錄 unsolicited push、query value、query response without value、unknown semantic、special app extension 與 no WF-RAC HTTP evidence；所有 upstream entry 的 local validation 仍為 `HARDWARE_TEST_PENDING`，`CONFIRMED_LOCAL` 維持 0。

WF-RAC-visible matrix 包含：`0x80/0x10` outdoor temperature（conversion 保留既有 `EVIDENCE_CONFLICT`）、`0x80/0x20` indoor temperature、`0x94` current-run energy（`(OP3 << 8 | OP2) * 0.25 kWh`，非 lifetime precision meter）、`0x11` outdoor shared compressor frequency、`0x90` current、`0x85` discharge/hot-gas temperature、`0x13` EEV raw/pulse、`0x81` indoor coil、`0x87` indoor coil outlet、`0x82` outdoor coil raw、`0xB1` TDSH-related raw、`0x7C` unresolved query value、`0x1E/0x1F` no-value responses、`0x0D/0x21/0x32/0x34/0x35` unknown semantic responses，以及 `0xF8` official-app-specific Home Leave extension。`0x34` 的 response 可能使用 OP3，因此不可遺失。

`0x7C` 不得建模為 `compressorProtectionActive`；`0x13` 不得把 raw／255 升級為 calibrated physical percentage。Direct CNS `0x0C` Defrosting 與 `0x05` set-temperature address 只保留 direct-CNS evidence，WF-RAC HTTP visibility 是 UNKNOWN，不得標為 confirmed unsupported，也不得推定 network capability。`0xF8` 的 app-specific round-trip 不代表 generic `OP1=0x00` writes 都安全；generic operation-data write 維持 OUT_OF_SCOPE。

Generic operation-data query 使用 `setAirconStat` 的 no-op COMMAND block 加 variable query segment，並且 response 必須從同一個 `setAirconStat` transaction 解析；不得改成 unauthenticated `getAirconStat` 或稍後再 poll。query command 沿用 R2 safety：writable set bits clear、command byte 5=`0xFF`、必要時 carry-through byte 8、query OP1=`0xFF`。Diagnostic query batch 只記 small/bounded 安全方向，不公開 upstream queue size 或實作 batching。以上 scope 不直接轉成 Matter endpoint，留待 Phase 0.3。

R3 remaining unknowns：`0x0C`／`0x05` WF-RAC HTTP visibility、`0x0D`／`0x21`／`0x32`／`0x34`／`0x35` physical meaning、`0x1E/0x1F` no-value exact reason、`0x7C` non-zero meaning/scope、`0xB1` physical semantics/scope、`0x13` calibrated scale、per-firmware support matrix，以及每個 operation-data field 的 user hardware validation。

## R4：Capability／Model Detection — CONFIRMED_UPSTREAM

`capability_detection_r4` 將 `ModelNrRaw`、wire-layout group、official-app capability group、retail model／series／capacity 與 local observed capability 分開。`ModelNrRaw` 來自 AirconStat RECEIVE `state[0] & 0x7F`，必須保存原值並以 raw value 作 official-app table lookup；wire encoding compatibility 不等於 feature capability identity。

Official-app groups：raw 0=`separate_2021`、1=`global_2022`、2=`high_end_for_japanese_2023`、3=`zt_2025`、64=`fdt_2023`，其餘 official-app fallback 為 `separate_2021`。17 個 flags 已寫入 machine-readable table：power consumption、vacant property、self-clean、LR wind、Entrust、LED、auto heating、Home Leave、temperature information、indoor/outdoor temperature、preset temperature auto、call-center phone、outdoor-temperature display、outside-Tokyo outdoor temperature、CoolHotJudge、preset range 2、operation-data。App flag 不是 wire presence proof，也不是 universal denial gate；特別是 raw 1 的 app `operation_data=false` 不得禁止 R3 generic query。

raw 3 的 official-app capability 是 `zt_2025`，即使部分 parser wire-layout 可視為 group 2，也不可覆蓋 raw capability lookup。Unknown raw 保留 raw、記錄 official-app fallback，但本 project 採 conservative policy：capability status UNKNOWN，不 optimistic enable optional writable features。

Optional gating 只作 capability hint：horizontal vane 需 `wind_direction_lr`、protocol encoding 與 local validation；Entrust／3D Auto 需 `entrust`；vacant property 仍 unresolved；Home Leave 受 `home_leave_mode` 與 tag 248 subcodes 27–32 gating；self-clean flag 不建立 remote start/status/control；LED app flag 不證明 JSON presence 或 writable semantics；energy 需 app hint、observed 0x94 與 local validation；preset range 仍 hardware pending。不得把這些直接轉成 Matter mapping。

Known protocol modes 是 auto、cool、heat、fan、dry，但 per-model support 保持 UNKNOWN，local support 為 `HARDWARE_TEST_PENDING`，不建立無 evidence 的 model mode table。firmType／firmware versions 只作 compatibility／quirk context，不推導 optional capability、retail model 或 size。Wire/capability group 不能可靠還原 SRK series、capacity、revision 或 exact indoor/outdoor model；retail model derivation 保持 false。每個 indoor device 保留自己的 ModelNrRaw 與 capability group，R3 shared outdoor data 不建立 shared capability owner。

R4 remaining unknowns：user-device raw model、每個 optional feature local validation、per-model HVAC modes、unknown future raw behavior、accepted setpoint ranges、LED／autoHeating semantics、app flags versus protocol presence across firmware、exact retail model/size，以及 hardware-specific capability differences。`CONFIRMED_LOCAL` 仍為 0。

## Evidence matrix

### CONFIRMED_UPSTREAM

Port/path、JSON envelope、HTTP/HTTPS variants、TLS/HTTP observations、上述五個 command family、Operator ID、`airconId` distinct identity、registration、mDNS `beaver/tcp`、Base64 AirconStat、CRC16-CCITT、state field family、indoor/outdoor temperature presence、energy-related field、serialized request、約 1 秒 pacing、esp-matter Room Air Conditioner、dual-MCU WF-RAC architecture、RL78 CNS/SPI bridge role、AirconStat ↔ CNS/SPI relationship、`_beaver._tcp`、七個 local handlers、timestamp sets module time、`getDeviceInfo` bootstrap、AirconStat COMMAND/RECEIVE dual block、variable-length block location、exact CRC16 parameters、no-request sentinel、state ↔ CNS/SPI mapping、write set-bit semantics、4-byte operation-data segment、unsolicited temperature segments、current-run energy behavior、multi-split per-indoor／outdoor-shared observations、one connection／約 1 秒 spacing、unexpected Content-Type 下仍可能有用的 JSON body，以及 transport status／protocol body separation。

### CONFIRMED_LOCAL

None.

### UNKNOWN／未來 mapping

各 device 的確切 transport variant、各 model 的 optional fields、非核心功能的最終 Matter mapping、最佳 poll interval、確切 retry policy、跨 model 的 energy semantics，以及 Google Home／Home Assistant controller UI 行為。
