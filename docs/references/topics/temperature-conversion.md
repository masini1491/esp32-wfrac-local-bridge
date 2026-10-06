# Temperature Conversion / Sensor Semantics

## Scope

R6 canonical temperature concepts、WF-RAC network lookup boundary、diagnostic sensor quality與 multi-split scope；包含software-only canonical ambient、bounded inferred discharge／indoor-coil candidates，不包含Matter mapping。

## Authoritative project conclusions

- Basic concepts 必須分離：`target_setpoint` 是 RECEIVE state[4]/DB2、raw/2 °C、0.5 °C resolution，屬 control；`controller_room_temperature` 是 RECEIVE state[5]/DB3、(raw−61)/4 °C、nominal 0.25 °C，屬 controller input；`external_room_temperature_injection` 是 COMMAND byte[5]/DB3，`0xFF` no injection，candidate encoding `round(T*4)+61`，write safety pending。Controller room state 不等於 0x80 indoor ambient alias。
- WF-RAC network `0x80` selector `0x10` 是 outdoor ambient、`0x20` 是 indoor ambient；canonical decoder 是 official Smart M-Air derived nonlinear lookup，2 tables × 256 entries，outdoor −50 至 43 °C、indoor −30 至 52 °C，fixed increment=false，存在 repeated/clamped low-end values。
- 舊 WF-RAC lookup／direct CNS formula conflict 已由 representation layer resolution 處理：CNS indoor `(raw−61)/4`、CNS outdoor `(raw−94)*0.25` 只適用 direct CNS/SPI，不能解 WF-RAC network OP2；exact bridge transform 仍未知，cross-layer difference 保留。Full 512 values 不嵌入 contract。
- `0x81` THI-R1 與 `0x87` THI-R3 是 per-indoor heat-exchanger sensors；portable `decode_inferred_indoor_coil_temperature()` preserves all raw bytes and identity. OP2=0 has no candidate; OP2 1–170 uses the inferred NTC divider inversion (R25=5200Ω、B=3900K、G=367、Rs=1912Ω) as integer deci-Celsius, rounded to nearest tenth with ties away from zero. OP2>170 is explicitly `inferred_extrapolated_candidate`; raw 170 is the highest current thermometer-supported anchor, not a precision calibration guarantee. `0x87` remains THI-R3 / secondary indoor-coil sensor, not a mode-independent universal “outlet”. Local hardware remains `HARDWARE_TEST_PENDING`.
- `0x82` THO-R1 是 shared outdoor raw-only channel，不重用 indoor curve，physical unit UNKNOWN。`0x85` shared outdoor discharge/hot-gas raw 可在 observed normal range 以 raw/2+32 °C 解讀，但 low-end semantics 未 universal establish，raw 保留。`0xB1` shared outdoor TDSH/discharge-superheat diagnostic raw-only，無 confirmed Celsius conversion。
- Diagnostic quality 必須區分 canonical ambient、bounded inferred candidate、extrapolated inferred candidate 與 raw diagnostic。Multi-split scope：0x80 indoor／0x81／0x87 per indoor；0x80 outdoor／0x82／0x85 shared outdoor。Feature/data scope 不等於 capability identity。

## Lookup implementation / provenance

`components/wfrac_protocol/src/ambient_tables.hpp` 現保存兩張256-entry exact deci-Celsius numeric tables；由 `blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c` 的 `custom_components/mitsubishi_wf_rac/wfrac/utils.py` normalization而來。MIT copyright／permission notice保留於table header；只引入numeric data，沒有port surrounding parser／comments。Pinned `rac_parser.py` 的 `_parse_temperatures()` 確認selector與OP2 index；沒有採用newer main。Machine-readable contract不嵌入512數值，指向implementation owner。

`decode_ambient_temperature()` 的integer output不損失pinned一位decimal精度；all-index host fingerprints／endpoints／clamped low-end／midpoints驗證data integrity，不代表local calibration。Controller room state、direct CNS representation、0x81/0x87 curve與其他UNKNOWN boundaries不變；local validation仍 `HARDWARE_TEST_PENDING`。

The indoor-coil candidate curve uses pinned `blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`; thermometer-supported anchors and limitations are evidence context, not precision calibration.

## High-risk invariants

Software-only `decode_inferred_discharge_temperature()` 只對 `0x85/OP1=0x10`、`OP2>=0x12` 回half-degree candidate=`OP2+64`（即raw/2+32°C）；even/odd保留0.5°C resolution。Low raw明確no-value，絕不採secondary30°C clamp。此 `inferred_diagnostic_candidate` 與canonical ambient lookup分離，不是calibrated Celsius／local hardware truth；完整raw保留。Frequency/current candidate與provenance見 [operation-data.md](operation-data.md)。

不得把 state[5]、0x80 indoor ambient、或任何 diagnostic raw 合併成 generic temperature；不得把 direct CNS formula 套到 WF-RAC OP2；不得宣稱 0x81/0x87 precision calibration、0x82 indoor curve、0xB1 Celsius 或 0x85 raw zero fixed 32 °C。

## Known unknowns / pending validation

Exact CNS-to-WF-RAC transform、external injection safety、model/firmware calibration、0x82 physical unit、0x85 low-end semantics、0xB1 conversion、unknown extra sensors 與 local hardware：`UNKNOWN`／`HARDWARE_TEST_PENDING`。TH R2、TH4、TH5 等沒有 known WF-RAC network mapping，不宣稱 universally unsupported。

**R6 dedicated Temperature Conversion / Sensor Semantics：COMPLETE**

## Related machine-readable contract

`temperature_semantics_r6`、`device_model_fields`、`operation_data_r3`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md) 與 [mhi-ac-ctrl-esp32.md](../sources/mhi-ac-ctrl-esp32.md)。
