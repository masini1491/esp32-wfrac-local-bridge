# Local WF-RAC PC validation evidence — 2026-09-02

## Scope and provenance

This dossier records sanitized `CONFIRMED_LOCAL` evidence from a Windows PC on a local LAN interacting with two real WF-RAC modules. It is not ESP32-C3/C6 hardware evidence, production-readiness evidence, or a universal claim about all WF-RAC firmware.

The evidence was obtained during a bounded PC probe on 2026-09-02. Device-specific identifiers are intentionally redacted and represented as `Device A` and `Device B`.

## CONFIRMED_LOCAL observations

- Both modules completed plain-HTTP `getDeviceInfo` and `getAirconStat` on port `51443` with HTTP `200` and application `result: 0`.
- Both modules reported the observed profile `firmType=WF-RAC`, wireless firmware `010`, and MCU firmware `131`.
- Each module's returned `airconId` / `macAddress` matched its corresponding physical module identity. Exact identifiers are not retained here.
- The `getAirconStat` Base64 payload was decoded by the existing parser contract into observed operation, settings, and telemetry data. This dossier does not redefine parser semantics.
- After Google Assistant controlled one module, its response evidence showed `updatedBy=GoogleAssistant` and a refreshed `expires`; the returned AirconStat reflected the module-reported state change.
- A PC local `setAirconStat` operation was accepted with HTTP `200` and application `result: 0`; subsequent module-observed state reported the setting change from `24.0°C` to `26.0°C`. The write response showed `updatedBy=pc-probe` and an observed `expires - timestamp` delta of `60s`.
- That successful PC write used an opaque client request `deviceId` with an existing Operator ID in a mirror-style flow. Exact identity values are not retained.
- After the PC write lock expired, the official Smart M-Air client issued a control accepted by the same module. The new writer refreshed `updatedBy` / `expires`, showing coexistence of the local writer and the official client under this observed lock lifecycle.

## Command acceptance versus physical actuation

- In additional `CONFIRMED_LOCAL` observations, Google Assistant or Smart M-Air could show an on/off control as completed while the indoor unit did not physically execute the requested change.
- Reopening the app and performing a fresh read could restore a display matching the observed physical state. This is an observation of client/readback behavior, not evidence that the implementation of the cache or readback path is understood.
- HTTP/application acceptance, `result: 0`, `updatedBy`, `expires`, write response, and immediate module-reported AirconStat are distinct from confirmed physical HVAC actuation. None alone proves that the indoor unit executed the command.
- The root cause of the occasional optimistic or stale client/UI state remains `UNKNOWN`; these observations do not attribute it to Wi-Fi, the module-to-indoor-unit link, lock collision, client behavior, firmware, or another single cause.

## SUPERSEDED diagnostic interpretation

An earlier Windows PowerShell 5.1 probe using native `curl.exe --data` produced HTTP `501` with `Not supported this command`. Byte-length comparison and a UTF-8 no-BOM file sent with `--data-binary @file` subsequently produced a successful result. The earlier `501` is therefore attributed to PowerShell/native-curl quoting and request-body corruption, not retained as a current firmware command limitation.

## Limits and non-generalization

- The observations are software/network evidence from a Windows PC to two real local modules. They do not establish ESP32-C3/C6 compile, flash, Wi-Fi, TLS, hardware validation, or physical actuation for every accepted command.
- Plain HTTP success does not establish HTTPS certificate behavior or a universal transport policy.
- The observed firmware profile and lock behavior must not be generalized to all WF-RAC models, firmware revisions, or accounts.
- No self-managed registration, persistent identity, cloud dependency, or production runtime readiness is established.
- This dossier records observations only; it does not authorize future write commands or hardware execution.

## Machine-readable contract boundary

`specs/wf-rac-contracts.json` describes universal software/protocol contracts and deliberately keeps `current_confirmed_local_count` at `0` while target local validation remains `HARDWARE_TEST_PENDING`. It has no scoped representation for this PC-only, device-profile-specific evidence. The machine-readable contract and validator therefore remain unchanged: this dossier and the reconciled topics retain the dated PC observations without promoting them to ESP32-C3/C6 hardware validation or universal firmware behavior.

## Privacy and redaction boundary

No Wi-Fi credentials, Operator ID, account or device UUID, exact LAN IP, exact MAC, exact `airconId`, authorization material, or other private-network/account identifier is stored in this dossier. Writer labels such as `GoogleAssistant` and `pc-probe` are retained because they are non-secret diagnostic labels.

## Reconciliation pointers

Use this dossier as the local provenance source when reconciling the directly related `api-envelope-discovery.md`, `tls-transport.md`, `airconstat-fixed-state.md`, `request-composition.md`, `identity-registration.md`, and `timestamp-time.md` topics. Topic conclusions remain authoritative for their own contracts; this dossier only supplies date-stamped local observations and their limits.
