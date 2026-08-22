# Timestamp / Time Architecture

## Scope

R5 canonical time architecture record，涵蓋 WF-RAC timestamp、module clock、timezone、expires、ESP-IDF system time 與 future Time Provider boundary；不包含 runtime implementation。

## Authoritative project conclusions

- WF-RAC request timestamp 是 mandatory、numeric、non-zero Unix seconds；firmware 會把 parsed timestamp 寫入 module time global。Timestamp 不是 ordinary informational field；client 可能成為 module time source。Future request 必須取得 current best-available Unix seconds，不得使用固定 timestamp fallback。
- 目前未建立 freshness window、monotonic requirement、clock skew、replay protection、cryptographic authentication 或 nonce semantics；timestamp 不得被宣稱為 authentication token、replay protection 或 cryptographic nonce。
- `updateAccountInfo.contents.timezone` 是 per-account metadata，與 UTC Unix epoch 分離；exact runtime effect、expires representation／lifetime／duration／refresh 與 timezone participation 仍 UNKNOWN。`expires` 只能作 diagnostic/advisory metadata，不是 universal scheduling constant。
- Clock correction 是 project architecture inference：只有 device 明確回報 expired/unregistered refusal，才 bounded 地處理自己的 self-managed registration；不得刪除 unknown accounts、清空 account table 或猜測 ownership。
- ESP-IDF evidence：RTC timer 可跨一般 reset／sleep保持，但 power-on reset 會重置；high-resolution timer 不跨 reset。software/watchdog reset 不等於 power loss，power loss 後也不能由 NVS checkpoint 知道 exact elapsed wall time。`time()`、`gettimeofday()`、`settimeofday()`、`adjtime()` 可使用；SNTP 是 optional acquisition／correction mechanism，不是 system-time prerequisite。
- Future conceptual boundary：trusted/best-available time source → Time Provider → WF-RAC Protocol → Transport。Time quality 需能區分 trusted、estimated/degraded、unavailable，但 exact runtime enum 與 C++ API 尚未設計。
- Daily local control 不要求 Internet、DNS 或 NTP。Persisted checkpoint 不是 trusted current time，只能作 lower-quality degraded estimate。External battery RTC 目前未選定且非 current architecture requirement。Future timestamp representation 必須 64-bit capable。

## High-risk invariants

不得把 timezone 加減到 Unix epoch；不得把 SNTP、Internet、DNS、NTP 或 external RTC 變成 daily-control prerequisite；不得將 fixed timestamp、32-bit-only architecture 或 checkpoint 當成可信現在時間。

## Known unknowns / pending validation

Hidden freshness/security behavior、timezone effect、expires semantics、clock correction、trusted time source、time-quality enum、power-cycle interaction 與 local hardware：`UNKNOWN`／`HARDWARE_TEST_PENDING`。

**R5 dedicated Time Architecture record：COMPLETE**

## Related machine-readable contract

`time_architecture_r5`、`api_envelope.request_fields.timestamp`、`identity_and_registration`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md) 與 [esp-idf.md](../sources/esp-idf.md)。
