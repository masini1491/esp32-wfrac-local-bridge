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

**User-authorized corrective reconciliation（2026-10-05）：**
- 使用者已明確授權本既有 Hot Stage 的 software-only Matter mapping / adapter contract，以及為消除 current authority / current-truth drift 所需的最低充分 project-document correction；這是既有 Stage revision，不是新 task，也不授權 Matter runtime / hardware。
- Current `AGENTS.md` 仍 generic 禁止 Matter Adapter / Matter runtime。先最小修正為：允許本 Stage 的 **software-only Matter mapping / adapter contract + host/compile validation**；actual Matter stack/runtime、commissioning、fabric、network/hardware execution與未授權相鄰 scope仍禁止。不得寫成「TASKS 本身產生 authority」。
- `docs/references/sources/esp-matter.md` 目前仍寫 Phase 0.3 尚未開始／本次不建立 Matter mapping，與 current Hot 衝突；修成 provenance-only source dossier，保留 pinned revision / license / authority boundary，不保存 stale task-status instruction。
- Matter mapping已成為獨立 retrieval intent；新增最低充分 `docs/references/topics/matter-rac-mapping.md`（或等價 focused topic），保存 project-admitted mapping semantics、pinned Matter authority、UNKNOWN/unsupported boundary與 provenance pointer，不複製整份 upstream source。
- `docs/roadmap.md` 與 `README.md` 的「Matter mapping 暫緩」current-status wording修成：**software-only mapping / adapter contract進行中；actual Matter runtime / commissioning仍暫緩**。
- `docs/architecture.md` / `specs/wf-rac-contracts.json` 只有在 current canonical truth實際需要時才改；「完整 Matter mapping尚未完成」本身仍為真，不為形式改寫。
- 完成上述 corrective reconciliation後再進 mapping implementation；若 current authority仍有無法消除的衝突，STOP，不得以 TASKS覆蓋更高層 project authority。

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
