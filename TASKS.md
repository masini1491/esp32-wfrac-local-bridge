# Unfinished work queue

## Hot — Add bounded 0x81/0x87 indoor-coil temperature candidate decoder

**Goal：** 在既有 operation-data classifier 與 typed read-side decoder pattern 之上，為 WF-RAC `0x81`（THI-R1）與 `0x87`（THI-R3）新增 software-only、read-only、raw-preserving 的 indoor heat-exchanger temperature candidate decoder；使用 pinned upstream thermistor-divider inversion，但清楚區分 evidence-supported candidate 與 high-raw extrapolation，且不得宣稱 precision calibration 或 `CONFIRMED_LOCAL`。

**User-authorized scope（2026-10-06）：**
- 使用者已明確要求繼續 software-only，並同意先整理 Cold 順序、只把第一個 bounded tranche promotion 進 `TASKS.md`。
- 授權只涵蓋 portable `wfrac_protocol` 的 `0x81/0x87` typed read-side candidate conversion、host tests、必要 topic/spec/README/roadmap reconciliation與既有 compile-smoke/CI integration。
- 不授權 hardware/network、operation-data write、Device Model/Matter/runtime consumer integration、`0x82`/`0xB1`、vane/3D Auto、advanced fixed-state或其他 residual admission。

**Pinned evidence / evidence level：**
- Primary WF-RAC authority：`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
  - `0x81` THI-R1 與 `0x87` THI-R3 均為 per-indoor heat-exchanger sensors；live multi-split evidence顯示它們隨各自 indoor unit 狀態變化。
  - Candidate conversion使用 divider inversion：
    `byte = G * Rs / (Rs + R(T))`
    `R(T) = R25 * exp(B * (1/T - 1/298.15))`
    with `R25=5200 Ω`, `B=3900 K`, `G=367`, `Rs=1912 Ω`.
  - Upstream parser對 `OP2=0` 回 no temperature；byte domain有效條件為 `0 < OP2 < G`，而 WF-RAC network byte本身為 0..255，因此 project-side唯一 reachable mathematical divider-limit invalid byte為 0。
  - Upstream reports representative curve outputs including raw 47≈5.5°C、75≈17.0°C、129≈34.1°C、170≈45.8°C、186≈50.4°C、252≈72.1°C。
  - raw 75 has direct thermometer corroboration (16.8°C measured vs 17.0°C curve); raw 170 is the highest current thermometer-supported anchor recorded by project authority。**raw >170 is explicitly extrapolated / inferred** and is not precision calibration。
  - Cold-end frost threshold is indirect/confounded evidence; do not force the curve to a 1°C cut-out anchor.
- Current project authority already classifies `0x81/0x87` as per-indoor, raw-preservable, bounded NTC candidate, with `>170` extrapolated/inferred and local validation `HARDWARE_TEST_PENDING`.
- Software arithmetic/tests do not promote this relation to `CONFIRMED_LOCAL` or calibrated physical truth。

**Scope / architecture：**
- Extend the existing `wfrac_protocol::OperationSegment` / operation-data owner; do not create a parallel parser.
- Add typed read-side value representation covering both `0x81` and `0x87`, preserving:
  - complete raw segment `[code, op1, op2, op3]`;
  - sensor identity / semantic kind;
  - per-indoor scope;
  - candidate temperature representation;
  - explicit quality sufficient to distinguish at least normal bounded candidate vs high-raw extrapolated candidate.
- Do not assume a universal OP1 selector. Pinned evidence includes `0x81` observed with selector `0x20` and `0x87` with `0x10`; current classifier is code-based. Decoder must not gate on a fabricated universal `OP1=0x10` rule.
- `OP2=0` must return explicit no-value / out-of-established-range disposition while preserving raw.
- For `OP2=1..170`, apply the pinned candidate curve and label it as inferred/bounded diagnostic candidate, not canonical/calibrated temperature.
- For `OP2=171..255`, the implementation may compute the same mathematically defined curve, but output metadata/quality must explicitly identify it as **extrapolated / inferred**. It must never be indistinguishable from the <=170 candidate region.
- Use a deterministic canonical representation with 0.1°C resolution (for example integer deci-Celsius) rather than float-only public semantics. Internal floating-point math is acceptable if needed for the logarithmic inversion, but rounding behavior must be explicit and deterministic.
- Preserve OP3 with no admitted conversion role.
- `0x87` must not be named as a universally mode-independent “outlet” physical semantic; retain THI-R3 / indoor-coil secondary-sensor wording unless stronger evidence exists.
- Do not derive superheat, delta-T, frost state, compressor state, efficiency or other secondary physical conclusions from these two values in this Stage.
- Do not inject values into Device Model, Matter, RuntimeOrchestrator, persistence, availability or control logic.

**Residual boundaries：**
- `0x82` remains shared-outdoor raw-only; do not reuse this indoor-coil curve.
- `0xB1` remains raw-only with Celsius conversion unestablished.
- `0x7C` remains UNKNOWN and not a protection boolean.
- `0x1E/0x1F` and unknown semantic codes retain current classification.
- Vane / horizontal / 3D Auto / advanced fixed-state remain outside this Stage.
- No operation-data write or exploratory control.

**Evidence/status reconciliation：**
- Update `docs/references/topics/operation-data.md` and `temperature-conversion.md` to separate:
  - canonical ambient/software decoders (`0x80/0x94`);
  - inferred outdoor candidates (`0x11/0x90/0x85`);
  - exact raw/network typed `0x13`;
  - bounded `0x81/0x87` indoor-coil candidate conversion with >170 extrapolation;
  - remaining raw/UNKNOWN diagnostics.
- Update `specs/wf-rac-contracts.json` minimally to record the typed decoder, constants/representation or stable contract pointer, raw preservation, per-indoor scope, `OP2=0` no-value behavior, >170 extrapolated quality, and `HARDWARE_TEST_PENDING`.
- README/roadmap only if current status or derived metrics materially change; do not inflate evidence level.

**Validation / completion gate：**
- Deterministic host tests must cover at least:
  - both `0x81` and `0x87`;
  - non-universal OP1 preservation/acceptance (including observed `0x81/0x20` and `0x87/0x10` shapes);
  - `OP2=0` explicit no-value/out-of-range with full raw preservation;
  - representative values around upstream points: 47, 75, 129, 170, 186, 252, with deterministic 0.1°C rounding;
  - boundary distinction 170 vs 171;
  - high-raw outputs clearly tagged extrapolated/inferred;
  - unsupported code, null output and alias/reset semantics consistent with existing `OperationValueError` pattern;
  - classifier remains per-indoor and does not become canonical/calibrated physical truth;
  - regression that `0x82` is not decoded through the indoor curve.
- Existing host/protocol regression remains green.
- Materially affected C6/C3 ESP-IDF compile/link gates remain green; no firmware/network/hardware execution.
- Library readiness = `PRESERVED`.
- `git diff --check` PASS; README physical-line metrics, if changed, are recomputed from final tracked content.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back; final report states candidate/extrapolated semantics, evidence limitations, remaining residual diagnostics, validation evidence and result SHA.

**Hard exclusions：**
- No precision-calibration claim.
- No `CONFIRMED_LOCAL` promotion.
- No `0x82` reuse of the indoor curve.
- No frost/protection/superheat inference.
- No hardware/network validation.
- No Device Model / Matter / runtime consumer integration.
- Do not execute the hardware Cold Candidate.
