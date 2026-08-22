# Timestamp / time

## Scope

只整理目前 repository 已存在的 timestamp、module clock、timezone 與 expires evidence；R5 dedicated record 尚未開始。

## Authoritative project conclusions

Request timestamp 必須存在、non-zero、Unix seconds。上游 module 會把收到的 timestamp 寫入 time global，module 沒有 RTC，因此 client 可能成為 module time source；timezone 儲存於 account metadata。Timestamp 不只是普通 payload field。

## High-risk invariants

不得在本檔自行加入 ESP-IDF time provider、NTP、checkpoint、64-bit 或 power-loss architecture；日常控制不依賴 Internet/NTP。

## Known unknowns / pending validation

Timezone exact effect、expires calculation／duration／refresh、aggTm/time-derived metadata 與可信 local time source：`UNKNOWN`／`HARDWARE_TEST_PENDING`。

**R5 dedicated Time Architecture record：PENDING**

## Related machine-readable contract

`api_envelope.request_fields.timestamp`、`identity_and_registration`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md)。
