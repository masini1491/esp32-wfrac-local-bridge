# Unfinished work queue

## Hot — Advanced fixed-state read-side guards

**Goal:** Add bounded protocol RECEIVE-only typed observations and negative semantic guards for evidenced fixed-state fields without asserting physical state, universal capability, or write/control support.

**Authorization and sequencing (2026-10-08):** User instructed ordered execution of five software-only stages. Docs reconciliation, vertical vane, and horizontal vane / Entrust read-side are canonically completed. This is the sole current Hot; Device Model / Matter mapping remains Cold.

**Evidence / authority:**
- Current project `AGENTS.md`, `docs/references/README.md`, `docs/references/topics/airconstat-fixed-state.md`, `docs/references/topics/capability-model.md`, source/tests, and `specs/wf-rac-contracts.json`.
- Pinned upstream `blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`, `docs/wf-rac-module-reference.md` §4.1, §4.2, §4.4, §4.6, §6.4, and `custom_components/mitsubishi_wf_rac/wfrac/rac_parser.py`.
- `state[6]` error: high bit first determines M-family with low-seven decimal code (including M00); otherwise zero is no-error `00`, nonzero is E-family. Do not conflate E and M or interpret code as universally diagnosed fault reason.
- `state[8] & 0x08` provides observed CoolHotJudge polarity, but full semantic/command-byte8 carry relationship remains unresolved; do not infer operation mode or physical heating/cooling.
- `state[9] & 0x02` is per-indoor compressor **demand** observation, not outdoor compressor running state or operation-data `0x11` frequency, especially on multi-split units.
- `state[10] & 0x01` is Vacant-related observed flag. Meaning, model support, occupancy inference and writable behavior require separate evidence; `vacant_property` app capability hint does not prove physical occupancy.
- `state[15] & 0x01` is NOT a reliable self-clean cycle-state observation: primary upstream reports it stayed clear during an IR-started full cycle, and no app remote-start route was established. Preserve raw, do not expose self-clean active/started/controllable as factual.
- `state[7] & 0x10` can echo command byte8; not compressor state. `state[13,14,16,17]` remain UNKNOWN. Do not infer new meanings for model-specific/reserved bits.

**Implementation scope:**
1. Reconcile current protocol RECEIVE state ownership and admit only minimum typed raw-preserving read-side representation for fields with evidence. Keep unresolved distinctions explicit; not every field needs a physical-value decoder.
2. Add bounded pure/portable read-side API/tests for error family/code, CoolHotJudge observed flag/polarity, per-indoor demand bit, Vacant observed bit. Preserve full raw bytes and avoid making optional-feature capabilities implicit.
3. Establish explicit **negative self-clean guard**: raw `state[15]` can be retained, but never project it as verified cycle active/idle or add reset/start control; if a typed field would imply reliability, prefer raw + caveat rather than a misleading boolean.
4. No `state[7]` compressor inference; retain command-echo observation only where evidence warrants. Unknown bytes and flags must remain unknown.
5. Update scoped fixed-state topic, contract/validator/negative regressions and roadmap only where materially necessary. Existing horizontal/vertical vane decoding and current basic HVAC/device model semantics must remain unchanged.

**Validation / completion:**
- Deterministic C++ protocol host tests cover error `00/E/M` precedence including M00, read-only raw preservation, CoolHotJudge polarity without physical effect, indoor demand independent of power and compressor frequency, Vacant bit independent of model-capability hints, and self-clean bit negative evidence; no writable command or Device Model/Matter projection.
- Existing Device Model host, Python syntax, contract validator and applicable Python tests PASS. Run `git diff --check`. If production `components/` changes, run materially affected C6/C3 compile/link CI on exact candidate SHA using isolated PR/candidate route; do not require local Windows ESP-IDF/MSYS2 recovery. Confirm source participation and six matrix outcomes.
- No actual firmware execution, Wi-Fi/TLS/WF-RAC/physical hardware tests, toolchain repair, registry changes or secrets.
- Keep library readiness `PRESERVED`, hardware `HARDWARE_TEST_PENDING`, no new `CONFIRMED_LOCAL`.
- On all gates PASS commit/promote legally, close `TASKS.md` to `EMPTY`, and canonical read-back. If required evidence fails/unavailable, preserve candidate and STOP without misclassifying environment failure as SOURCE.

**Hard exclusions:** No control/write semantics, reset/start self-clean, physical occupancy inference, outdoor compressor-running projection, invented error diagnosis, Device Model/Matter integration, speculative advanced features, or adjacent Cold work.
