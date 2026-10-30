# GATEA-D2 — HTTP-client Stage A source dependency / Kconfig review (2026-10-30)

**Scope:** 2026-10-30 09:37 Asia/Taipei Codex-reported **read-only** WSL agent follow-up, plus ChatGPT independently fetched upstream source/Kconfig at the pinned SDK revision. **Not a configure/build/link or runtime acceptance.** This is sanitized provenance staging under project `AGENTS.md`; it does not supersede SDK source or `TASKS.md` authority.

- Target: `masini1491/esp32-wfrac-local-bridge@8ad9e2f6d1ea7d7544096e5a1959924ffb9d34d7` (Codex reported current local HEAD after **safe fetch + ff-only** synchronization from `ee1f6b0938b1ef2e595b172edd2ac6a96e6e5c3`, 8 commits behind, 0 ahead, clean worktree, linked-worktree pointers and unfinished-operation checks PASS; git diff --check PASS). ChatGPT independently confirmed GitHub remote `main` at `8ad9e2f6d1ea7d7544096e5a1959924ffb9d34d7` when reviewing this handoff. Local checks are Codex-reported, not ChatGPT local observations.
- Shared Playbook: `masini1491/ai-development-playbook@29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70d`.
- Pinned SDK: `espressif/esp-idf@7101770dc6db2667b3c477cc31365dd1acd6db4e`. Codex reports this time it independently isolated the SDK Git identity and found a clean SDK worktree, plus previously selected lwIP/mbedTLS submodule pins matching the declared baselines. **Correction to 09:27:** that initial SDK Git query was affected by project `GIT_DIR` contamination and must NOT be cited as independent SDK revision/clean proof; today's 09:37 report supersedes that limited claim. ChatGPT has not independently inspected Codex's local SDK/submodule checkout.

## Fixed upstream source evidence independently cross-checked by ChatGPT

- [`components/esp_http_client/CMakeLists.txt`](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/CMakeLists.txt) — for `IDF_TARGET=linux`, public requirements `linux`, `esp_event`; private requirements `tcp_transport`, `http_parser`. Non-Linux substitutes `lwip` for `linux`.
- [`components/tcp_transport/CMakeLists.txt`](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/tcp_transport/CMakeLists.txt) — `esp-tls` required, with conditional `esp_timer`/lwIP behavior depending on Linux build system/config.
- [`components/esp-tls/CMakeLists.txt`](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp-tls/CMakeLists.txt) — `mbedtls` required, plus private parser/timer dependencies and conditional Linux/lwIP behavior.
- [`components/esp_http_client/Kconfig`](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/Kconfig) — `CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS` default `y`; `CONFIG_ESP_HTTP_CLIENT_ENABLE_CUSTOM_TRANSPORT` default `n`, requires deliberate `y` for the prospective fixture.
- [`components/esp_http_client/esp_http_client.c`](https://github.com/espressif/esp-idf/blob/7101770dc6db2667b3c477cc31365dd1acd6db4e/components/esp_http_client/esp_http_client.c) — source contains original `http_parser_execute()` calls and custom transport injection/routing; no runtime-path proof follows from reading these source lines.

## Acceptance limits and next decision

**Scope-qualified outcome:** `STAGE_A_SOURCE_DEPENDENCIES_CONFIRMED__GENERATED_KCONFIG_AND_LINK_CLOSURE_PENDING`. Safe WSL Git sync, local SDK identity, submodule identity and source checks are Codex-reported; listed upstream CMake/Kconfig/parser evidence is separately source-confirmed by ChatGPT.

**UNPROVEN / NOT_RUN:** Generated effective Linux Kconfig, complete build component graph, exact source/object/link closure, `IDF_TARGET=linux` configure/build/link, test ELF execution, injected custom transport behavior, HTTP parser integration at runtime, real TLS/socket/physical cleanup, aggregate deadline and L1/L2/L3. No production file changes, test-only harness creation, SDK patch, Docker, CI, hardware or real networking reported.

**Authorization:** The existing task admits stages A–C test-only source/build/link with Codex as implementation actor, D zero-egress **design only**, subject to current launch/permission/revision/STOP gates. The immediately preceding Codex launch was explicitly Stage A read-only only and has completed. Do not automatically start B/C or treat upstream source checks as build/link results. A new bounded launch must reread current `AGENTS.md`/`TASKS.md`, verify current remote identity and establish any required operation-level permission before attempting B/C; no stage E binary execution permitted. Preserve prior Windows-native permission STOP as historical and scoped to that execution surface.

Parent sole Hot `GATEA-D2`; Gate A `PENDING/UNPROVEN`; Gate B `HOLD`; Stage4 `COLD`; library readiness `PRESERVED`.
