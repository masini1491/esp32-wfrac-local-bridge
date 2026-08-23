# Unfinished work queue

此檔只追蹤尚未完成、Blocked、Deferred 或待驗證工作。完成項目應直接移除；Git history 才是完成紀錄。不得因看到本檔而自行執行未明確授權的工作。

## P1 — Pre-hardware correctness

- Fix `wfrac_api` response string decoding so empty strings / zero-capacity caller outputs cannot cause a null write. Add targeted malformed/empty-output tests.
- Replace unsafe end-of-buffer pointer-addition checks in JSON/Unicode parsing with remaining-length checks; add truncated `\uXXXX` / surrogate-pair tests.
- Refine ESP-IDF HTTP/TLS failure classification so DNS/socket/connect/connection-timeout evidence is not incorrectly collapsed into generic `tls_failure`; preserve the rule that the classifier never invents `scheme_mismatch`.
- Align `EspHttpTransport` PEM certificate length contract with ESP-IDF v6.0.2 semantics. Explicitly document/test whether `cert_len == 0` means NUL-terminated PEM and how nonzero lengths include the terminator.
- Make response-parser outputs atomic on failure: `parse_api_response_envelope()` and `parse_get_device_info_contents()` should commit metadata only after full success, matching the `getAirconStat` parser behavior.

## P2 — Response parser / library hardening

- Reject decoded embedded NUL in identity/C-string outputs such as `airconId` / `macAddress`, or otherwise make the public contract unambiguous.
- Allow unknown JSON numeric values to be safely skipped using full valid JSON number grammar while keeping known integer fields such as `result` / `apMode` strict integers.
- Compare known JSON keys and `command` by decoded string value so valid escapes such as `\uXXXX` do not break semantic matching.
- Preserve and validate raw UTF-8 string bytes correctly instead of re-encoding each non-ASCII byte as a Unicode code point; add valid and malformed UTF-8 tests.

## P3 — Maintenance / contracts

- Reset `encoded_size` to zero consistently on early failure in portable operation-query builders.
- Document `AirconFrameView` / segment-view lifetime and immutability expectations: views reference caller-owned raw storage, and validated bytes must not be mutated behind cached CRC-valid state.
- Add the permanent `TASKS.md` queue rule to `AGENTS.md`.
- Add the library-extraction invariant to `AGENTS.md`: keep the monorepo for now, preserve portable-core extractability, and prevent ESP-IDF/FreeRTOS/Wi-Fi/NVS/Matter types from leaking into portable public APIs.
- Fix architecture wording that still describes `wfrac_transport_espidf` as the only ESP-IDF platform adapter now that `wfrac_time_espidf` also exists.

## Deferred

- **Phase 1C #4H — Diagnostic Wi-Fi + read-only WF-RAC bootstrap firmware skeleton.** Defer until the P1 pre-hardware correctness items above are fixed and validated, so first hardware/network diagnostics are trustworthy.
- **First real WF-RAC read-only probe / hardware validation.** Start only after the diagnostic bootstrap firmware is compile-ready and local scheme/TLS trust configuration is explicit. Initial probe should remain bounded and read-only (`getDeviceInfo` first).
