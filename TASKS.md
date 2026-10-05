# Unfinished work queue

## Hot — Add Matter Room Air Conditioner mapping / adapter contract

**Goal：** 以 current `wfrac_device_model::DeviceState` / `CommandIntent` 為唯一 WF-RAC-side semantic owner，建立 software-only、host-testable 的 Matter Room Air Conditioner mapping / adapter contract；只 admission current evidence足以支持的 mapping，不啟動 actual Matter stack/runtime。

**Current Matter authority / fixed evidence：**
- Project-pinned Matter reference：`espressif/esp-matter@1e913f65d7f40eb8617be46b195dcacc9a80c165`（Apache-2.0）。
- Pinned Room Air Conditioner device type：
  - device type ID `0x0072`
  - device type version `3`
  - mandatory endpoint surface：`Descriptor`、`Identify`、`On/Off`、`Thermostat`
  - Room Air Conditioner endpoint adds On/Off `dead_front_behavior`
- Do not silently replace this pin with newer esp-matter main/current generated data. Upstream refresh is a separate reconciliation decision, not part of this Stage.

**Scope / architecture：**
- Add a dedicated Matter-adapter boundary above `wfrac_device_model`; Device Model must not depend on Matter and must not expose esp-matter / CHIP types。
- Prefer a small project-owned, host-testable mapping contract/component (for example `wfrac_matter_adapter` or an equivalent clearly owned boundary) that depends on Device Model, not vice versa。
- The mapping layer may model the minimum Matter RAC semantics needed for deterministic tests, but must not duplicate the full Matter SDK or create a second protocol specification。
- Fresh-read only the pinned Matter files / project references needed to resolve each mapping. If a mapping remains ambiguous after minimum-sufficient evidence, keep it explicit `UNKNOWN` / unsupported rather than inventing behavior。
- Build an explicit mapping table / contract for at least current Device Model surfaces:
  - power；
  - HVAC mode；
  - target temperature；
  - controller room temperature；
  - availability / unknown-state presentation boundary；
  - fan setting；
  - Matter-originated write intent → Device Model `CommandIntent` where evidence is sufficient。
- Preserve state ownership:
  - Matter-facing observed attributes derive from authoritative observed Device Model state, not optimistic desired/pending state；
  - pending / desired lifecycle remains owned by Device Model / RuntimeOrchestrator；
  - Matter adapter does not build WF-RAC requests, parse JSON/AirconStat, manage Operator ID, transport, retry, discovery or sockets。
- Preserve exact/nullable/unsupported semantics. Unknown Device Model values must not be converted to fabricated Matter defaults。
- Temperature conversion must remain lossless where representable; do not introduce new floating-point ownership if current centi-Celsius representation can be used directly。
- Fan / fan-only / dry / vane / 3D Auto / outdoor temperature / energy / vacant-property / diagnostics / error-code mappings remain `UNKNOWN` unless this Stage obtains direct pinned Matter + current WF-RAC evidence sufficient for a bounded standard mapping. Do not use custom/vendor clusters merely to close a gap。
- If the mapping becomes an independent retrieval intent, add the minimum focused Matter-mapping topic under `docs/references/topics/` and keep `docs/references/sources/esp-matter.md` as provenance rather than duplicating full upstream content。

**Governance reconciliation：**
- Current `AGENTS.md` generically prohibits Matter Adapter / Matter runtime. This Stage authorizes only the **software-only mapping / adapter contract + host/compile validation** exception. Reconcile that wording minimally while keeping actual Matter stack/runtime, commissioning, fabric, network/hardware execution and unrelated scope prohibited。
- Update architecture / roadmap / README / machine-readable contract only where current truth, canonical ownership or derived metrics actually require it；do not perform a broad docs rewrite。

**Validation / completion gate：**
- Deterministic host tests cover admitted projection + Matter-originated command mappings, unknown/unsupported handling, non-optimistic observed-state ownership, and exact temperature semantics as applicable。
- Device Model remains Matter-independent; dependency direction is `Matter adapter → Device Model`。
- If a new portable component is added, include it in the existing compile-smoke path where appropriate so C6/C3 compile/link integration remains proven without actual Matter runtime。
- Existing host/software CI remains green；materially affected ESP-IDF C6/C3 compile/link gates remain green。
- No actual Matter SDK runtime, commissioning, fabric, Wi-Fi, WF-RAC network or hardware execution occurs。
- Library readiness = `PRESERVED`。
- `git diff --check` PASS。
- README physical-line metrics, if affected, are recomputed from final tracked content。
- On success, return `TASKS.md` to minimal `EMPTY`, commit/push, perform canonical remote read-back, and report exact mapping scope, remaining UNKNOWN mappings, validation evidence and result SHA。

**Hard exclusions：**
- No actual esp-matter node/endpoint runtime, Matter commissioning/fabric, Matter over Wi-Fi/Thread execution, hardware, flashing, serial, WF-RAC network I/O, NVS, persistence or production deployment。
- Do not execute any Cold Candidate。
- Do not move Matter types into `wfrac_device_model` or lower protocol/client layers。
- Do not introduce custom/vendor cluster decisions for unresolved optional WF-RAC features in this Stage。
