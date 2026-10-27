# GATEA-D3 — pinned ESP-IDF total-attempt-deadline feasibility assessment

- Date: 2026-10-27 (Asia/Taipei)
- Task lineage: Stage 3 / GATEA-D2; follows the scoped-accepted Stage 2R synthetic-only corrective.
- Evidence scope: **READ_ONLY_SOURCE_ASSESSMENT**, not host fake-test, SDK compile/link, network, physical deadline, cancellation safety or hardware validation.
- Local repository observed: `masini1491/esp32-wfrac-local-bridge@d51f7422e41f3dd5687eec99e2a722d7b5ca98dd`.
- Exact frozen official ESP-IDF reference: `espressif/esp-idf@7101770dc6db2667b3c477cc31365dd1acd6db4e`.
- No file, code or hardware/source mutation was performed by this assessment; this inbox note and its TASKS pointer are coordination/evidence staging only.

## Scoped source evidence (frozen revisions)

1. Project `components/wfrac_transport_espidf/src/esp_http_transport.cpp`, blob `74f2cd6b20c904b1803c216e276e58df8721cfbf`: `EspHttpTransport::perform` sets `client_config.timeout_ms=request.timeout_ms`, uses blocking `esp_http_client_perform(client)`, assigns stack-owned `ResponseCollector` as `user_data`, and clears/cleans up after return. On body overflow, event handler records `overflow=true` and returns `ESP_OK` on later data; this does NOT stop transport drain. No local overall monotonic cutoff or independent synchronous-call abort is present in this implementation.
   - https://github.com/masini1491/esp32-wfrac-local-bridge/blob/d51f7422e41f3dd5687eec99e2a722d7b5ca98dd/components/wfrac_transport_espidf/src/esp_http_transport.cpp
2. SDK public `esp_http_client.h`, blob `ffafee00e4a829643b71f16e7ed542bdfbe5a01b`: `timeout_ms` is documented as a network timeout; `is_async` is explicitly documented **HTTPS-only for now**. Blocking `esp_http_client_perform` is the default, and its documentation warns against simultaneous calls on one handle. A public `esp_http_client_cancel_request` API DOES exist.
   - https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/include/esp_http_client.h#L210-L260
   - https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/include/esp_http_client.h#L315-L383
3. SDK `esp_http_client.c`, blob `69c3d4687b082f2390cf0eab2ccbf426d8be25c7`: `esp_http_client_fetch_headers` loops `esp_transport_read(..., client->timeout_ms)` until headers arrive; `esp_http_client_perform` then loops body reads (`esp_http_client_get_data`) based on chunk/completion, each again calling transport read with the same `client->timeout_ms`. There is no checked monotonic request-level deadline inside those loops in this reviewed revision. Individual per-read timeout does not prove a physical whole-attempt wall-time bound under continued positive bytes/trickle data.
   - https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/esp_http_client.c#L1471-L1638
4. SDK `esp_http_client_cancel_request` implementation: checks connected state, calls `esp_transport_close`, **then calls `esp_http_client_connect` again using the same context**; it is not a simple one-way cancel-and-drain primitive. Neither the reviewed API documentation nor implementation establishes cross-thread cancellation/quiescence/teardown safety for a concurrently active blocking `perform` call. This is an **unproven contract**, not a finding that the function is absent.
   - https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/esp_http_client.c#L494-L515

## Decision / implication

- **CONFIRMED at static source level:** Existing project blocking `perform` + per-network-operation timeout does **not establish L1** controllable *total* physical attempt budget. Collector overflow is fail-closed for returned data, but does **not establish L2** bounded physical stop; a return or timeout check after `perform` cannot constrain its already elapsed blocking duration.
- **NOT PROVEN:** cancellation called concurrently is safe, immediately stops I/O, yields terminal callback quiescence, or permits releasing stack-owned collector/client safely. Thus **L3 lifecycle/cleanup**, `D_physical`, `D_cleanup` and callback quiescence remain PENDING/UNPROVEN.
- **Architecture implication (conditional):** Do NOT prescribe `is_async=true` as dual HTTP/HTTPS solution, and do NOT use `esp_http_client_cancel_request` as a physical abort/quiescence guarantee based on its name. Do NOT solve a physical deadline by moving blocking `perform` into a detached worker with unbounded lifetime. A separate bounded/cancellable I/O lifecycle and caller-owned lifetime proof is required before any implementation claim.
- **Lower-contract option:** A caller-level *reported* `timed_out` after a blocking operation returns is possible as classification, but this **does not satisfy** the current Stage 3 truly enforceable aggregate physical deadline; it would require an explicit user-approved scope reduction.
- **Next minimum admissible work candidate, not authorized by this note:** a bounded, software-only feasibility/design contract review of a common HTTP+HTTPS ownership and deadline mechanism, with explicit `start→active→terminal→quiescence→release` transitions, zero detached/unbounded workers, no unsafe concurrent handle access, deterministic fake-SDK boundary tests for trickle headers/body, overflow, cancel/close and terminal cleanup. If no documented mechanism can prove a common physical bound and cleanup, **STOP / report design blocker** rather than claim Gate A PASS. Formal test/implementation authorization and reproducible exact candidate required before running a new SDK campaign.

## Preserved states

- Stage 2R: `STAGE2R_TEST_ONLY_CORRECTIVE_ACCEPTED__TRACE_LIMITED` (unchanged).
- Original HTTPS positive map mismatch: UNRESOLVED.
- Gate A: PENDING/UNPROVEN; Gate B: HOLD; Stage 4: COLD; library readiness: PRESERVED.
- Host runtime, real HTTP/TLS/network, SDK compiler/linker, CI and hardware: **NOT_RUN in this assessment**.
