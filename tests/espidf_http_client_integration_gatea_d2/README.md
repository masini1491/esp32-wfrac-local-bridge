# Build-only HTTP client integration candidate

Pinned ESP-IDF: 7101770dc6db2667b3c477cc31365dd1acd6db4e.
The six compiled cases cover HTTP/HTTPS parser routing, injected-handle retention with close failure, and rejection of cleanup success as physical-close success. They have NOT_RUN status. Real SDK factories, parser, dispatcher and event APIs remain linked; no default event loop is created. This is no proof of TLS CA/SNI/hostname validation or event queue lifetime.

## Stage D: design only

Before any future execution, independently inspect the linked ELF and all socket/DNS/TLS egress entrypoints. Use a separately authorized deny-by-default host boundary that denies socket/connect/send/receive including loopback, with a negative control demonstrating denial before the real fixture is started. Cover dynamic-library/syscall escape paths, built-in TCP/SSL factories and DNS. A container network-none flag alone is insufficient. Verify actual process restrictions and fail closed on any missing boundary, unexpected descriptor, attempted egress, timeout or evidence gap. Only then may a separately authorized one-start six-case runtime witness be considered. No guard is implemented here.

L1 aggregate deadlines, L2 atomic publication and L3 callback quiescence/resource retirement remain UNPROVEN. Synthetic close counters do not establish physical resource termination. The negative oracle is confined to in-memory evidence.

## Corrective witness contract

The first callback ordinal/return is recorded independently of later explicit close and cleanup returns. The negative cleanup oracle requires an actually observed synthetic close failure plus cleanup success; it makes no physical/quiescence inference. Six JSON case records and one total record are produced only on future authorized execution; allocation/init failures become failed cases. Linux exits nonzero unless all six pass. No runtime has been performed.

The ignored sdkconfig.defaults must be explicitly included by path (`git add -f -- tests/espidf_http_client_integration_gatea_d2/sdkconfig.defaults`) when staging this candidate; no global ignore rule change is required.
