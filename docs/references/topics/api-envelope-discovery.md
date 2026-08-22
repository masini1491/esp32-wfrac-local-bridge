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
- Portable typed builders 已實作 `getDeviceInfo`、`getAirconStat` 與 `setAirconStat` envelope；它們使用 caller-owned buffer、JSON escaping、64-bit Unix timestamp，並在 wrapping 前驗證 AirconStat CRC。actual HTTP request 仍 pending。

## High-risk invariants

不可把 synchronous HTTP 放進 Matter callback；不得把 cloud/MQTT handlers 當 local API；不得以 global `result = 2` 解讀所有 commands。

## Known unknowns / pending validation

Local transport、discovery、exact polling／retry tuning 與 firmware-specific handlers：`HARDWARE_TEST_PENDING`。

## Related machine-readable contract

`api_envelope`、`result_and_error_taxonomy`。

## Provenance pointers

見 [homebridge-mhi-wfrac.md](../sources/homebridge-mhi-wfrac.md)、[homey-wfrac.md](../sources/homey-wfrac.md)、[mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md)。
