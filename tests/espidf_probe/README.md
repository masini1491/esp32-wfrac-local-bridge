# Gate A5 — SDK-linked, compile-only probe

ESP-IDF v6.0.2 `7101770dc6db2667b3c477cc31365dd1acd6db4e`,
esp-lwip `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.

The six existing compile cells first report six allowlisted options from their
actual `build-ci/sdkconfig`. The isolated fixture copies that generated config,
builds against the same SDK, and compares those six effective settings again.
Only filtered settings are logged; full configuration and firmware are not uploaded.

`app_main` retains a reference graph; it invokes no DNS, callback, network or TLS
operation. The strict compiled graph includes actual `tcpip_try_callback` and
`dns_gethostbyname_addrtype` symbols, stable static registration/fence storage,
`ERR_INPROGRESS`/immediate completion, expired-result discard and explicit
`ERR_MEM` admission/fence branches. Storage is never freed/recycled; this fixture
does not implement a production lifetime or synchronization policy.

Evidence tier: `LINK_ONLY`. Callback completion ordering, delayed/absent callbacks,
late results, duplicate-host registrations, actual queue-full admission, fence
delay/failure and safe token recycling are all `NOT_RUN / UNPROVEN`.
No supported offline SDK scheduler has been established for these C3/C6 builds.
No generic fake scheduler, emulator, new worker, SDK patch or hardware execution
is introduced. Compile/link cannot prove quiescence, reclaim, close latency or
hard synchronous caller-return bounds. Gate A feasibility remains PENDING;
Gate B remains ON HOLD.
