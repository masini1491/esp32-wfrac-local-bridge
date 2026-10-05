# Unfinished work queue

## Hot — Add bounded inferred outdoor diagnostic conversions

**Goal：** 在既有 operation-data raw/classifier與 canonical `0x80`／`0x94` decoder之上，新增 software-only、read-side、明確標示 **INFERRED candidate** 的 `0x11` compressor frequency、`0x90` operating current與 bounded `0x85` discharge-temperature conversions；不得把候選公式升格為 calibrated／CONFIRMED_LOCAL physical truth。

**User-authorized scope（2026-10-05）：**
- 使用者已明確要求執行下一個 software-only residual diagnostics tranche。
- 授權只涵蓋 portable protocol read-side candidate conversion、host tests、必要 topic/spec/README/roadmap reconciliation與既有 compile-smoke/CI integration。
- 不授權 hardware/network、operation-data write、Device Model/Matter diagnostic integration、vane/3D Auto control或其他 residual diagnostic admission。

**Pinned evidence / evidence level：**
- Primary WF-RAC authority：`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
  - Exact pinned source records live WF-RAC responses for `0x11` / `0x90` / `0x85` and says their values are consistent with MHI-AC-Ctrl formulas, but explicitly states two operating points are **not calibration** and the formulas remain **INFERRED**。
  - Multi-split observation classifies `0x11` / `0x90` / `0x85` as shared-outdoor values。
- Secondary cross-reference：`hberntsen/mhi-ac-ctrl-esp32@cb00cd54525acea9fbafdfee178a657eb6667faa`（MIT），direct CNS/SPI semantics only，不是 WF-RAC HTTP authority。
  - `0x11` candidate：12-bit value from `((DB10 << 8) | DB11) & 0x0FFF`，physical formula high part ×25.6 + low byte ×0.1 Hz；project WF-RAC representation等價可用 exact deci-Hz candidate。
  - `0x90` candidate：`OP2 × 14 / 51 A`。
  - `0x85` candidate：normal-range `OP2 / 2 + 32°C`；secondary implementation對 low raw做30°C clamp，但 project authority已明確判定 low-end semantics未 universally established，因此 **不得引入 universal clamp**。
- 本 Stage的任何 derived physical value evidence label保持 `INFERRED` + local `HARDWARE_TEST_PENDING`；software tests不升級 evidence level。

**Scope / architecture：**
- 延伸既有 `wfrac_protocol` operation-data owner與 `OperationSegment` raw preservation；不另建平行 parser。
- 新 API / value type必須讓 caller從型別或metadata明確看出「inferred diagnostic candidate」，不得與 canonical ambient／exact energy共用會混淆 evidence strength的名稱。
- `0x11`：
  - 只對 current project-supported response shape admission；selector/high bits需符合 evidence-supported outdoor diagnostic form（至少 `(OP1 & 0x30) == 0x10`），否則 unsupported/no physical value。
  - 保留 raw。
  - canonical implementation representation優先 exact **deci-Hz integer candidate**，避免 float-only。
  - 依 pinned evidence equivalent semantics：`deci_hz = ((OP1 - 0x10) << 8) + OP2` only when OP1 is in the supported 0x1x selector form；implementation需避免 underflow/wrap並明確驗證 accepted OP1 range。
- `0x90`：
  - 只對 evidence-supported selector form admission；保留 raw。
  - formula保持 exact rational candidate `OP2 × 14 / 51 A`。Public representation可使用 numerator/denominator、raw count + fixed rational metadata，或其他不丟失 exact relation的 integer form；不得以 rounded float作唯一 canonical candidate。
  - 不由此推導 electrical power、voltage、COP或energy。
- `0x85`：
  - 只對 evidence-supported selector form且 `OP2 >= 0x12` admission bounded candidate。
  - exact candidate `OP2 / 2 + 32°C`，優先以 half-degree count／等價整數表示。
  - `OP2 < 0x12` 必須回傳 explicit unsupported/out-of-established-range disposition並保留 raw；**不得**以30°C clamp、32°C fixed value或其他 fabricated physical temperature代替。
- Existing `OperationData` classifier可新增能表達 inferred/bounded-derived availability的 quality，但不得把 `0x11` / `0x90` / `0x85`標成 canonical/calibrated/exact physical truth。
- 不把 candidate diagnostics注入 Device Model、Matter、RuntimeOrchestrator或 persistent state。
- 不把 App capability flags當 value-presence或calibration proof。

**Residual boundaries：**
- `0x13` EEV仍 raw，不轉 percentage/full-open fraction。
- `0x81/0x87` thermistor candidate curve本 Stage不實作。
- `0x82` physical unit保持 UNKNOWN。
- `0xB1` Celsius conversion保持 UNKNOWN/raw-only。
- `0x7C`不得建模 protection boolean。
- `0x1E/0x1F`、`0x0D/0x21/0x32/0x34/0x35`、`0xF8`維持 current semantics。
- vane / horizontal / 3D Auto / advanced fixed bytes不在本 Stage。

**Evidence/status reconciliation：**
- 更新 `docs/references/topics/operation-data.md` 與 `temperature-conversion.md`，清楚分開：
  - canonical exact software decoders（0x80/0x94）；
  - inferred/bounded diagnostic candidates（本 Stage）；
  - raw/UNKNOWN residual diagnostics。
- `specs/wf-rac-contracts.json`只做最低充分 machine-readable status/semantics更新；不得改 `current_confirmed_local_count` 或將candidate標成 `CONFIRMED_LOCAL`。
- README/roadmap只在 current status或 derived metrics受影響時更新。

**Validation / completion gate：**
- Deterministic host tests至少覆蓋：
  - `0x11` zero/representative/high-bound candidate arithmetic、accepted selector/high-nibble boundary、unsupported selector、raw preservation；
  - `0x90` zero/representative/max raw exact rational relation，證明不因 integer truncation／float rounding改變 canonical candidate semantics；
  - `0x85` `0x12` boundary、representative even/odd raw（0.5°C resolution）、high raw、`<0x12` explicit no-value，並 regression證明沒有30°C clamp；
  - null/unsupported output reset semantics與其他 residual UNKNOWN/raw classifications不退化；
  - evidence-quality metadata不把 candidate冒充 canonical/calibrated。
- Existing host/protocol regression保持 green。
- Materially affected C6/C3 ESP-IDF compile/link gates保持 green；不得執行 firmware/network/hardware。
- Library readiness = `PRESERVED`。
- `git diff --check` PASS；README metrics若受影響從 final tracked content重算。
- On success，`TASKS.md` 回到 minimal `EMPTY`，commit/push，canonical remote read-back；報告 candidate conversions、evidence limitations、remaining residual diagnostics、validation evidence與 result SHA。

**Hard exclusions：**
- No operation-data write (`OP1=0`) or exploratory control。
- No hardware/network validation。
- No canonical/calibrated claim for `0x11` / `0x90` / `0x85`。
- No low-end `0x85` clamp。
- No Device Model / Matter diagnostic integration。
- Do not execute hardware Cold Candidate。
