# TLS / transport

## Scope

HTTP／HTTPS branches、transport detection 與目前 TLS observations；R7 dedicated policy 尚未開始。

## Authoritative project conclusions

- Legacy branch observed HTTP `:51443`；newer WF-RAC-HTTPS branch observed HTTPS `:51443`。Current direction is HTTPS-first detection，historical Homey behavior曾 HTTP-first fallback。
- Observed upstream conditions include self-signed certificate、TLS 1.2、ALPN `http/1.1`、`Connection: close`、keep-alive disabled、one connection at a time，以及可能重要的 header casing。
- These are branch observations，not universal firmware requirements。Certificate verification policy remains UNKNOWN；不得把 verify-off、TLS minimum 或 security level 寫成 production rule。

## High-risk invariants

Transport detection 與 TLS policy 未經 hardware validation 不得實作或宣稱完成。

## Known unknowns / pending validation

Device transport variant、exact TLS compatibility/security policy 與 local behavior：`HARDWARE_TEST_PENDING`。

**R7 dedicated TLS policy：PENDING**

## Related machine-readable contract

`api_envelope.transport_variants`、`result_and_error_taxonomy.tls_verification_policy`。

## Provenance pointers

見 [homebridge-mhi-wfrac.md](../sources/homebridge-mhi-wfrac.md)、[homey-wfrac.md](../sources/homey-wfrac.md)。
