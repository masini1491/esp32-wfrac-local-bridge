# Unfinished work queue

## Hot — Vertical vane protocol core

**Goal:** Build a bounded, portable software-only vertical wind-direction (up/down swing and fixed-position) receive decode and safe COMMAND encoder using current pinned WF-RAC evidence, with deterministic host and applicable C6/C3 compile/link regression. No hardware/network activity.

**Authorization:** User requested ordered execution of the committed software-only sequence on 2026-10-08; Docs reconciliation has completed canonical acceptance. This is the *only* current executable Hot. Adjacent horizontal vane/3D Auto, advanced fixed-state and Device Model/Matter work remain Cold.

**Authority/evidence:**
- Project Phase 1C and current `AGENTS.md`, `docs/references/README.md`, `docs/references/topics/airconstat-fixed-state.md`, `docs/references/topics/request-composition.md`; project source/tests are canonical implementation truth.
- Pinned primary upstream `blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c` (`custom_components/mitsubishi_wf_rac/wfrac/rac_parser.py` command/receive vertical wind mapping; `docs/wf-rac-module-reference.md` §4.2–4.3).
- RECEIVE: `state[2]&0xC0==0x40` indicates up/down swing; fixed position is `(state[3]&0xF0)>>4` for raw 0..3 → position 1..4, ignored when swinging. Unknown/reserved combinations must not be optimistically interpreted as valid fixed positions.
- COMMAND is a **paired two-byte direction selection**, not two independently executable controls: upstream `WindDirectionUD → (command[2], command[3])` masks `0:(0xC0,0x80)`, `1:(0x80,0x80)`, `2:(0x80,0x90)`, `3:(0x80,0xA0)`, `4:(0x80,0xB0)`. The command set bits are byte2 bit7 and byte3 bit7; values byte2 bit6 and byte3 bits5:4. Preserve all other fields at no-op safe default.
- The upstream app's full-state command path is **not** approval for unrelated full-state writes; existing project observed-baseline request composition and byte8 carry constraints remain authoritative.
- Firmware-specific physical vane behavior, actual actuation, capabilities and completion timing remain `HARDWARE_TEST_PENDING`.

**Implementation scope:**
- Add explicit typed vertical direction/swing receive representation in portable `wfrac_protocol` (preserve raw byte details and explicit unknown/invalid encodings). Define a bounded safe single-intent vertical direction COMMAND encoding for swing and position 1–4 in both byte 2 and 3, reusing existing safe default/EncodeError patterns. Do not infer that unknown raw codes map to position 1.
- Connect this command encoding to existing **protocol-level** safe raw request composition only if the relevant current API allows a narrow non-Device-Model addition without expanding intent/runtime ownership; otherwise scope is protocol codec + host framing regression and document deferred integration. Do not expand `CommandIntent`, DeviceState, Device Model, runtime orchestration, API, Matter adapter or actual network writes during this Stage.
- Amend current fixed-state topic and machine-readable contract only as necessary to reflect admitted software-only semantics; preserve `CONFIRMED_UPSTREAM` vs `HARDWARE_TEST_PENDING`.
- No new dependencies from portable core into ESP-IDF/FreeRTOS/Wi-Fi/Matter.

**Validation / completion:**
- Deterministic receive cases: swinging even if byte3 position bits are nonzero; fixed positions 1–4; unknown/reserved byte2 swing patterns and byte3 invalid high nibble handled conservatively; unrelated state fields and original raw preserved. Do not conflate swing on/off with position.
- Deterministic command cases: exact paired command byte2/byte3 for 0..4; invalid/unknown inputs and null pointer fail closed with no partial actionable command; byte5 remains `0xFF`, no unrelated set bits. Test encoding → AirconStat composition → parsing / CRC where safely available.
- Existing command encoding and device-model request composition remain regression-green; no new Device Model/Matter command admission.
- Python contract validator/targeted tests, C++17 host regression, materially affected ESP32-C6/C3 compile/link matrix, and `git diff --check` PASS or clearly classified STOP/evidence gap.
- Library readiness `PRESERVED`; no actual Wi-Fi/TLS/HTTP/WF-RAC/flash/hardware execution; no `CONFIRMED_LOCAL` promotion.
- On success, close `TASKS.md` to minimal `EMPTY`, commit/push and canonical exact-SHA remote read-back; report actual validation revisions and child delegation/profile override.

**STOP / exclusions:** No horizontal vane, 3D Auto, advanced fixed-state, Device Model/Runtime/Matter mapping, speculative capability enable, exploratory writes, inferred production physical actuation, or adjacent Cold work. If upstream command/receive mapping conflicts with current project evidence, stop for bounded reconciliation before any production semantic change.
