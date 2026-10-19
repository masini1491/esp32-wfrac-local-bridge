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

## D2-P1-T — mbedTLS/BIO reference graph (compile/link only)

`app_main` additionally retains the address of `wfrac_d2_tls_link_anchor` but never
invokes it. That test-owned graph references SSL client defaults, required verify
mode, empty CA-chain config, setup, empty hostname, BIO, and ordered context/config/
CA disposal. No real certificate/key, hostname, IP, credential, handshake, socket
connect, target execution or network is introduced. The existing A5 graph remains.

The fixture explicitly requires `mbedtls`. Header-only `decltype` checks establish
the upstream `mbedtls_net_poll/close` prototypes, without creating unresolved calls
or claiming target implementations. Actual `net_send/recv/recv_timeout/free`
addresses are retained; timeout/free are NOT called by this fixture.

Each C3/C6 build runs `verify_tls_link.py` **after a successful link**, using the
actual generated sdkconfig and compile_commands, preprocessing the actual fixture
command to check `MBEDTLS_NET_C` undefined and `MBEDTLS_CONFIG_FILE` selection.
It verifies pinned SDK/submodule identities and selected port bytes, rejects an
upstream networking source in the compile database, inspects the actual port object,
fixture relocations, final ELF and linker cross-references. This is stronger than
prototype compilation or symbol grep alone. No ELF is executed.

```sh
idf.py -C tests/espidf_probe -B /absolute/output/c6 -D IDF_TARGET=esp32c6 build
idf.py -C tests/espidf_probe -B /absolute/output/c3 -D IDF_TARGET=esp32c3 build
```

SDKCONFIG remains build-local. Use the existing pinned SDK/toolchain; do not
download/install, patch SDK or change security settings to make a build pass.
The post-link verifier prints an allowlisted JSON `D2_P1_LINK` record per actual
target, source/object identity, available symbols and ELF SHA256. Classification
is **SDK_COMPILE_ONLY**, never TLS/callback/network/hardware runtime evidence.
The existing six scope-selected CI cells each also build this A5 fixture from
their actual firmware config; no workflow/selector or SDK config is modified.
Publishing such a PR requires the Stage's separate CI budget preflight.

Host exact-pointer/length and borrowed-owner negatives are documented in
`tests/host_cpp/README_deadline_d2_p1.md`. They remain SYNTHETIC_PASS only.
Native failed-close/quiescence, TLS identity verification, request timing and
hardware remain UNPROVEN / NOT_RUN; Gate A PENDING and Gate B HOLD.
