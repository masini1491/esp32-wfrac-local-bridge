# WF-RAC protocol research

## Evidence terminology

`CONFIRMED_UPSTREAM` means explicitly supported by a pinned reference. `CONFIRMED_LOCAL` requires this project’s own hardware/log evidence. `INFERRED` is a reasoned hypothesis. `UNKNOWN` is unresolved. `HARDWARE_TEST_PENDING` requires the user’s WF-RAC hardware/runtime validation.

## Initial upstream evidence — CONFIRMED_UPSTREAM

- TCP port `51443`
- API path family `/beaver/command/<command>`
- `getAirconStat`
- legacy HTTP and newer HTTPS support
- newer HTTPS uses a self-signed certificate
- Operator ID and remote/controller registration concepts
- `updateAccountInfo` and `deleteAccountInfo`
- remote slot count has an upper limit
- status polling and active commands should avoid collision
- upstream integrations include retry/reconnect/polling coordination

These are upstream research facts, not claims of local hardware confirmation. The project’s protocol/runtime validation remains `HARDWARE_TEST_PENDING`; no `CONFIRMED_LOCAL` claim is made.
