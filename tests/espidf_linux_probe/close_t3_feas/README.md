# CLOSE-T3-FEAS — independent Linux loopback BUILD/LINK ONLY

Never execute this fixture ELF. No TCPIP/netif startup, socket, peer, connection,
packet, DNS/TLS/HTTP or hardware operation is admitted. The address-only anchor
contains no runtime calls, including if linked into an application. This is not
the existing A6b/T1/T2A fixture and does not change its config or runtime inputs.

Pins: ESP-IDF v6.0.2 `7101770dc6db2667b3c477cc31365dd1acd6db4e`;
esp-lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.

In an already installed, separately inspected Docker `--network none` container,
with source and SDK read-only, capabilities dropped, no-new-privileges and a
separate executable writable `/out`:

```sh
idf.py --preview -C /src/tests/espidf_linux_probe/close_t3_feas \
  -B /out/build -D IDF_TARGET=linux build
```

SDKCONFIG is local to the independent build directory. The post-link Python
verifier checks the actual generated Linux/IPv4/lwIP/loopback config, resolved
preprocessor macros, pinned Kconfig/lwipopts/netif source blobs, compiled netif
input, anchor object relocations, retained loopback ELF symbols and actual
producer/consumer cross-reference map. `app_main` disassembly must contain only
address references, not calls or tail jumps. No ELF loader/runtime is invoked.
The current proof targets the actual native Linux x86-64 toolchain; it does not
claim ARM host or C3/C6 build evidence.

```sh
python -B tests/espidf_linux_probe/close_t3_feas/verify_build.py
python -B tests/espidf_linux_probe/close_t3_feas/verify_build.py --inspect /out/container.json
```

Twelve synthetic guard cases check config, macros, missing references, accidental
runtime calls and unsafe container network/privilege/source/SDK-overlay settings.
Docker inspect evidence is configured isolation only, not observed proof of no
possible escape. Read-only `/sys/class/net`, `/proc/net/route` and namespace
identity may be recorded without sending packets or starting lwIP.

Source mapping is SOURCE_CONFIRMED; successful native build/link is only
SDK_COMPILE_ONLY. Fixture execution and every socket/connected-close test remain
NOT_RUN. No SDK_RUNTIME_CONFIRMED, FIN/ERR_MEM, PCB reclamation, callback
quiescence, aggregate L1/L2/L3 or caller-return guarantee is established.

Current CI selector predicts callback=true, dns=false, compile=false for this
path, but the existing callback job builds/runs only the parent fixture, NOT
this independent fixture. Its green result would not validate this new build.
Do not publish a triggering PR until Actions scope/budget is separately approved;
do not change workflows/selectors or dispatch an SDK matrix to fill this gap.
Gate A PENDING/UNPROVEN; Gate B HOLD; Stage 4 Cold; Library readiness PRESERVED.
