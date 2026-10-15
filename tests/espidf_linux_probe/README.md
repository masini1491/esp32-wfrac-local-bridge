# Gate A6b — offline pinned SDK Linux runtime probe

Pins: ESP-IDF v6.0.2 `7101770dc6db2667b3c477cc31365dd1acd6db4e`,
esp-lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.

The existing ESP-IDF CI container builds the preview Linux target with
`CONFIG_LWIP_ENABLE=y`. It reports that target's generated sdkconfig independently
of C3/C6, including FreeRTOS mode, core-locking and mailbox settings, then runs the
linked ELF with a 20-second external watchdog. Each initialization/event/gate wait
has a 5-second test budget; the deliberately pending fence is checked with a
50-millisecond FreeRTOS wait. These are test watchdogs, not production deadlines.

Three in-memory cases execute real SDK `tcpip_init` and `tcpip_try_callback`:
nested callback/fence order on the same TCPIP task; a blocked callback and pending
fence across caller wait expiry with retained static storage; actual callback
admission/fence ERR_MEM under bounded queue saturation and subsequent drain.
ERR_MEM may come from message allocation or mailbox admission; the accepted count
and generated mailbox capacity are printed, without claiming allocator fault injection.
Core callbacks record events and signal SDK synchronization primitives; only the
app task prints. No netif, DNS lookup, socket or TLS call is made.

Owners remain static and are never recycled or freed. Process exit is not SDK
worker teardown. The exit marker is a test callback marker, not a DNS registration
clear event. The deferred fence is not proof of DNS callback quiescence.
DNS completion/cancellation, duplicate-host resolver behavior, never-arriving DNS,
safe token recycling, shutdown, close timing and hard caller-return bounds remain
NOT_RUN / UNPROVEN. Gate A feasibility stays PENDING, Gate B ON HOLD.

Initial feasibility evidence: candidate `09aee250e1b4f3c264539b4f0163aef9bd02fc25`,
Actions run `38590885407`, Linux job `115945125012`: build/link and one real
TCPIP callback smoke PASS. Linux generated configuration: lwIP enabled,
FreeRTOS SMP disabled, core locking disabled, thread-safety checking disabled,
mailbox 32, mDNS-query support enabled, DNS servers 3, SO_LINGER disabled.
This configuration evidence does not imply any DNS/mDNS query was executed.
The final three-case probe requires its own exact-candidate CI evidence.
