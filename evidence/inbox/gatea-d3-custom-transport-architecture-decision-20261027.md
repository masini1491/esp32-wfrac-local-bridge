# Gate A / D3 — Deadline-aware transport architecture decision (read-only)

Date: 2026-10-27 (Asia/Taipei)  
Scope: **READ_ONLY_PINNED_SDK_ARCHITECTURE_DECISION / PROVISIONAL_DESIGN**, not approved implementation, compile, real network, physical timing, hardware or Gate A acceptance.  
Project observed: `masini1491/esp32-wfrac-local-bridge@48d15c675584d1817367206b0ca574f828029155` (project transport source unchanged since `d51f7422`).  
Pinned vendor SDK: `espressif/esp-idf@7101770dc6db2667b3c477cc31365dd1acd6db4e`.  
Predecessor: [pinned SDK feasibility](gatea-d3-pinned-sdk-total-deadline-feasibility-20261027.md); Stage 2R remains `STAGE2R_TEST_ONLY_CORRECTIVE_ACCEPTED__TRACE_LIMITED`.

## Architecture decision

**Conditional preferred candidate / not implementation admission**: investigate `esp_http_client_config_t.transport` custom-transport injection, backed by a **single-owner, shared monotonic absolute-deadline guard** covering both HTTP plain TCP and HTTPS authenticated TLS. Keep ESP-IDF HTTP request/response parser rather than immediately writing a new HTTP/1.1 parser. Explicitly regard this as a falsifiable lowest-change feasibility candidate, **not** a proven hard physical deadline.

**Why the built-in modes are insufficient:** `esp_http_client_perform()` is blocking by default; vendor `timeout_ms` is used per connect/read/write and repeated inside header, chunked and length-based body loops. `is_async` documentation is HTTPS-only. A detached worker, caller-only post-return deadline classification, or simultaneous `esp_http_client_cancel_request()` cannot prove `D_physical`, terminal callback quiescence and `D_cleanup`; the cancel implementation also reconnects after closing a socket. Prior independent evidence and original HTTPS positive-map hash mismatch remain unchanged.

## Pinned-vendor API / implementation evidence

- **Custom injection is real and OFF by default.** `esp_http_client_config_t.transport` exists only under `CONFIG_ESP_HTTP_CLIENT_ENABLE_CUSTOM_TRANSPORT`; its Kconfig definition says `default n`. `esp_http_client_init()` assigns `client->transport = config->transport`; `esp_http_client_connect()` uses the scheme transport from its internal list only if `client->transport` is unset. Project currently does not select a custom implementation.
  - [SDK Kconfig](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/Kconfig)
  - [SDK config header](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/include/esp_http_client.h#L243-L252)
  - [SDK client init / connect](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/esp_http_client.c#L864-L968)
- **Supported transport seam.** `esp_transport_set_func` provides connect/read/write/close/poll/destroy callbacks. `esp_transport_tcp_init()` and `esp_transport_ssl_init()` implement underlying plain and authenticated schemes. `esp_tls_conn_new_async` is documented for non-blocking TLS and `esp_tls_cfg_t.is_plain_tcp` also supports plain transport at the TLS abstraction layer.
  - [SDK transport API](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/tcp_transport/include/esp_transport.h#L161-L313)
  - [SDK tcp/ssl transport implementations](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/tcp_transport/transport_ssl.c#L571-L602)
  - [SDK esp_tls.h](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp-tls/esp_tls.h#L463-L545)
  - [Official v6.0 ESP-TLS documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32/api-reference/protocols/esp_tls.html)
- **Custom seam is NOT sufficient proof of a hard deadline.**
  1. Header and body read loops can receive indefinitely many positive trickle chunks. A wrapper must check **remaining absolute budget before every underlying connect/read/write/poll**, reject attempts after expiry, and bound each call individually; merely forwarding a per-read `timeout_ms` does not establish an aggregate bound.
  2. `esp_http_client.c::http_dispatch_event_to_event_loop` unconditionally calls `esp_event_post` with `CONFIG_ESP_HTTP_CLIENT_EVENT_POST_TIMEOUT`, Kconfig default **2,000 ms**. This may add waiting **outside** transport I/O callbacks. `http_on_body` dispatches both a synchronous user event and a posted event, then returns 0; it **ignores the synchronous handler return**, so changing the response collector to return `ESP_FAIL` cannot by itself guarantee early stop on overflow. Queued events can contain client-handle identifiers, requiring explicit subscription/lifetime review.
  3. `esp_http_client_init` internally configures its own SSL transport. A separately provided custom `config.transport` does not automatically inherit that internal TLS CA/SNI/ALPN configuration. TLS identity validation and authenticated error classification must be explicitly preserved in any custom scheme backend; no skip-verification, downgrade or undocumented fallback.
  4. `esp_tls_hostname_to_fd` calls `getaddrinfo()` **before** nonblocking socket connect; no aggregate deadline check is visible around this call. Numeric-IP-only precondition or a separately bounded resolution path must be proved, not assumed. Explicit numeric IPv4 support exists in the diagnostic plan, but this research does not narrow the whole project's hostname policy automatically.
  5. In the pinned `esp_tls.c`, a `non_block` plain TCP `connect()` returning `EINPROGRESS` can lead through `esp_tls_low_level_conn()` to a `1` plain-TCP return without a proven completed `SO_ERROR` check in that path. Do **not** equate a documented generic async API with a verified plain-HTTP connected state; inspect or test the actual SDK path before using it.
  6. Underlying TLS handshake, socket operations, event dispatch, cleanup, clock regressions and scheduling jitter all need bounded behavior or an explicit uncontrollable-overrun classification. Static source presence proves a candidate interface, not end-to-end WCET or physical cancellation.
  - [Client event dispatch & handler](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/esp_http_client.c#L190-L218)
  - [Client on-body semantics](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/esp_http_client.c#L354-L391)
  - [SDK event-post timeout Kconfig](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/Kconfig)
  - [SDK TLS resolver / connect](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp-tls/esp_tls.c#L221-L291)
  - [SDK TLS nonblocking state path](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp-tls/esp_tls.c#L390-L558)

## Proposed bounded ownership and deadline contract (hypothesis, NOT implementation)

- One synchronous owner of client + transport + collector for each physical attempt; no separate watchdog thread touching the same handle, no detached worker and no concurrent `cancel_request`.
- State candidates: `NEW → CONNECTING → WRITING → HEADERS → BODY → DECIDED → QUIESCENT → RELEASED`; error/expiry/overflow enter `STOPPING` then `QUIESCENT` only after all executing callbacks and underlying transport activity cease.
- Monotonic absolute deadline `T0 + budget`, tested by an injectable clock before each potentially blocking I/O and on each resulting transition. The physical limit must include connect, resolution if present, TLS handshake, all header/body traffic, callback/event overhead and resource teardown, with separately measured/contracted maximum overshoot. No `SUCCESS` may be declared after expiry or overflow. Absolute-deadline expiry must be sticky and reject new I/O.
- `D_result` (reported terminal decision), `D_physical` (no more operation-owned I/O), and `D_cleanup` (all resources released) must be **separately observable**. `D_result` alone is not a hard deadline guarantee. A connection that could have sent a write before timeout has an **unknown delivery** outcome, not an automatic safe-to-retry indication. Preserve no automatic retry/fallback behavior.
- Retain the existing stack-owned response collector only if SDK callback and event-loop lifetime are proven quiescent before return and cleanup; otherwise choose a safe, bounded ownership model. Do not hide unbounded retained state as a leak-free fix.

## Feasibility comparison / decision gates

| Candidate | Decision now | Missing proof |
| --- | --- | --- |
| Existing synchronous perform with single `timeout_ms` | **REJECT for hard aggregate bound** | Internal repeated read loops, event overhead |
| High-level `is_async=true` for both schemes | **REJECT as dual-scheme claim** | Pinned header documents HTTPS only |
| Independent watcher/worker + concurrent cancel | **REJECT without safety authority** | No proven cross-thread cancel/join/quiescence, double-use handle risk |
| **Custom deadline-aware `esp_transport` with existing HTTP parser** | **PREFERRED RESEARCH CANDIDATE — CONDITIONAL** | Physical bound including all non-I/O SDK calls; TLS/HTTP branch, callback lifetime, Kconfig and ownership |
| New common HTTP/1.1 parser + lower-level TCP/esp-tls stepper | **BACKUP RESEARCH CANDIDATE**, not committed | More code/parser surface, TLS semantics, protocol edge tests, license and long-term maintenance |

**Go/no-go:** Implementation admission is **NO-GO until** bounded offline seam experiments + exact-SDK source behavior establish each critical path. If the custom-seam cannot bound event posting/resolution/TLS/cleanup, do not proceed to source implementation on the strength of transport timeout alone. Consider smaller exact-SDK source-level proof or fallback parser only after concrete failing evidence, and obtain independent scope/authorization for that expansion.

## Next minimum evidence stage candidate — NOT ADMITTED

A single **design-verification-only** bounded fake-transport/source-level tranche, after fresh authorization:
- Freeze two scheme profiles, caller-owned clock and lifetime model; record exact SDK config / sources and test targets. No live network/hardware and no SDK build campaign by default.
- For both HTTP and HTTPS fake paths, demonstrate deterministic deadline cases: header trickle, chunked/body trickle, connect/handshake delay, read/write stalls, EOF, response overflow, sticky expiry and no post-timeout write, malformed/incomplete response, alias/ownership cleanup, clock regression and late/queued events.
- Prove no unexpected nested execution, no stale callbacks after release, no IO after STOP, correct TLS trust configuration boundary, and clear `D_result/D_physical/D_cleanup` evidence tiers; include deliberate wrong-guard / invalid-baseline controls.
- STOP if unsupported SDK hooks, event-loop unbounded wait, unverifiable plain TCP connect state, or a required change exceeds agreed scope; no silent promotion to new scheduler, hardware validation, invasive SDK patch, or redesigned HTTP protocol.
- Preserve original HTTPS map inconsistency and Stage 2R acceptance limits. A design-only PASS would still not constitute real physical deadline, SDK compile/link, runtime or Gate A PASS.

Current state: Stage 2R CLOSED (SYNTHETIC_TEST_ONLY / TRACE_LIMITED); Gate A PENDING/UNPROVEN; Gate B HOLD; Stage 4 COLD; library readiness PRESERVED.
