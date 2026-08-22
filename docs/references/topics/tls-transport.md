# TLS / Transport Security Policy

## Scope

R7 software-only transport/security policy；不包含 TLS runtime、HTTP client、certificate pin implementation 或 hardware validation。

## Authoritative project conclusions

- Port `51443` observed：older WF-RAC HTTP、WF-RAC-HTTPS HTTPS、WCBN4612L HTTPS；scheme per every firmware version UNKNOWN，不由 retail model推導。
- mDNS `_beaver._tcp` TXT empty、port 51443，不能宣告 HTTP/HTTPS scheme。Unknown device architecture 為 HTTPS-first，以 `getDeviceInfo` 作 bootstrap-safe probe，再 HTTP compatibility fallback；timestamp 仍有 module-clock side effect。
- Successful scheme 應 per-device persist/cache，normal requests 使用 last known scheme；只在 transport-level evidence rediscover，不得每次 poll probe。HTTPS 成功後 sticky：transient failure 或 application refusal 不得 silent downgrade to HTTP。
- Compatibility profile：preferred TLS 1.2、HTTP/1.1、initially no connection reuse、`Connection: close`；TLS 1.2 是 observed profile，不是 universal minimum。部分 newer device 有 legacy TLS handshake compatibility issue；exact min TLS/ciphers UNKNOWN，global weakening forbidden。
- Observed HTTPS self-signed certificate 與 IP/CN mismatch。Preferred direction 是 server identity verification（device-specific trust/pin strategy），exact mechanism pending hardware。Verify-off HTTPS 只能分類 `encrypted_unauthenticated_transport`，不是 authenticated/verified secure transport。HTTP legacy compatibility 的 confidentiality、server identity、active-attacker integrity 均 none。
- ESP-IDF supports certificate validation/configuration、ALPN、SNI、keep-alive controls 與 explicit insecure skip；platform capability 不等於 production policy。
- Local WF-RAC HTTPS evidence 不要求 mTLS；cloud MQTT security 是 separate boundary。Operator ID 是 application identity，不是 TLS authentication。Header casing compatibility observed，但 exact requirement pending。
- Failure layers 概念上分為 TCP connect、TLS handshake compatibility、TLS server identity、HTTP protocol、WF-RAC application refusal；certificate failure 不得自動觸發 plaintext downgrade。

## High-risk invariants

不得把 verify-off、TLS 1.0、SECLEVEL、特定 cipher、hostname skip 或 insecure ESP-TLS configuration 直接變成 universal production constants；不得用 application result/refusal 掩蓋 transport discovery，也不得以 public NTP 作 daily-control prerequisite。

## Known unknowns / pending validation

Per-firmware scheme、exact TLS/cipher profile、certificate uniqueness/stability/lifecycle、hostname handling、degraded HTTPS user policy、header casing、certificate validity-time behavior 與 local device：`HARDWARE_TEST_PENDING`。

**R7 TLS / Transport Security Policy：COMPLETE**

## Related machine-readable contract

`transport_security_r7`、`api_envelope.transport_variants`、`result_and_error_taxonomy.tls_verification_policy`、`time_architecture_r5`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md)、[homebridge-mhi-wfrac.md](../sources/homebridge-mhi-wfrac.md)、[homey-wfrac.md](../sources/homey-wfrac.md) 與 [esp-idf.md](../sources/esp-idf.md)。
