# API envelope / discovery

## Scope

WF-RAC local API boundary、handlers、discovery、request serialization 與 command-specific result parsing。

## Authoritative project conclusions

- `POST /beaver/command/<command>`，observed port `51443`。
- Required envelope fields：`apiVer`、`command`、`deviceId`、`operatorId`、non-zero Unix `timestamp`；`contents` handler-specific。Observed `apiVer` `1.0` 不是 universal firmware constant。
- Local command inventory：`getDeviceInfo`、`getAirconStat`、`setAirconStat`、`updateAccountInfo`、`deleteAccountInfo`；cloud/module handlers 不視為 local HTTP handlers。
- `getDeviceInfo` 是 bootstrap；response body 即使 Content-Type 不理想仍可能包含 useful JSON。Transport status 與 protocol body 必須分開解析。
- mDNS service 為 `_beaver._tcp`；advertised port 只作提示，expected API port 仍是 `51443`。
- Request 必須 serialized、one-at-a-time，約 1 秒 pacing；timeout/retry 是 upstream behavior，不是 universal constants。Result semantics 必須 command-specific。
- Reviewed 五個 local commands 都已有 portable request builder：`getDeviceInfo`、`getAirconStat`、`setAirconStat`、`updateAccountInfo` 與 `deleteAccountInfo`。builders 使用 caller-owned buffer、JSON escaping、64-bit Unix timestamp，並在 AirconStat wrapping 前驗證 CRC；project ESP-IDF runtime execution仍 pending。
- `getAirconStat` response parser 已將 contents 的 `airconStat` JSON string 經 Base64 decode、AirconStat structural parse 與 COMMAND／RECEIVE 雙 CRC gate 轉成 portable `AirconFrameView`；response `airconId` 目前 optional，其他 contents fields bounded-skip。這是 software validation；它本身不代表 portable implementation已在 ESP-IDF target上解析實機 response。
- Portable post-write verifier 只將 `setAirconStat` 的 `result: 0` 視為此 project command path 的 accepted response，接著由 caller-scheduled、bounded `getAirconStat` readback 交給既有 parser／Device Model reconciliation。accepted 不等於 physical actuation confirmed；readback count 是 caller policy，沒有硬編碼 settle delay、polling interval 或 universal retry constant。
- `CONFIRMED_LOCAL`：在兩台 `WF-RAC / wireless 010 / MCU 131` profile module，Windows PC 已收到 `getDeviceInfo`／`getAirconStat` 的 HTTP `200`、`result: 0` response；`getAirconStat` 的 Base64 payload可依既有 parser contract解讀。PC `setAirconStat` 亦獲 module 接受，並由後續 module-observed state 回報 `24.0°C → 26.0°C`。這是 command acceptance／module readback evidence，不單獨證明 physical HVAC actuation；也不驗證 ESP-IDF runtime、discovery、firmware-wide handlers、polling 或 retry tuning。見 [2026-09-02 PC local evidence](../sources/local-wfrac-pc-validation-2026-09-02.md)。

## High-risk invariants

不可把 synchronous HTTP 放進 Matter callback；不得把 cloud/MQTT handlers 當 local API；不得以 global `result = 2` 解讀所有 commands。

## Known unknowns / pending validation

ESP-IDF local transport、discovery、physical settle timing、exact polling／retry tuning 與 firmware-specific handlers：`HARDWARE_TEST_PENDING`。先前 PowerShell/native-`curl.exe` body corruption 造成的 HTTP `501` command-incompatibility interpretation已為 `SUPERSEDED`；它不是目前 firmware limitation。見 [2026-09-02 PC local evidence](../sources/local-wfrac-pc-validation-2026-09-02.md)。

## Related machine-readable contract

`api_envelope`、`result_and_error_taxonomy`。

## Provenance pointers

見 [homebridge-mhi-wfrac.md](../sources/homebridge-mhi-wfrac.md)、[homey-wfrac.md](../sources/homey-wfrac.md)、[mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md)。
