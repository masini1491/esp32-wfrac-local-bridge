# Temperature conversion

## Scope

目前已存在的 basic state、operation-data 與 CNS/SPI temperature evidence；R6 dedicated research 尚未開始。

## Authoritative project conclusions

- Basic target temperature 是 AirconStat state path；controller room temperature 是 state／DB3 path；operation-data temperature 不可與 basic state 混用。
- `0x80` indoor/outdoor evidence 存在；outdoor conversion 保留 `EVIDENCE_CONFLICT`：WF-RAC nonlinear lookup 與 direct CNS `(raw - 94) * 0.25` 不自行選邊。
- `0x81`／`0x87` coil evidence 存在；`0x82` outdoor coil raw 存在但 conversion 未建立。Rough／approximate formulas 不升級為 confirmed calibration。

## High-risk invariants

Protocol presence 不等於 local reporting；不得把 conflict 解決成 universal formula 或把 operation-data raw 變成 precision sensor。

## Known unknowns / pending validation

Model／firmware conversion、coil calibration 與 local hardware readings：`HARDWARE_TEST_PENDING`。

**R6 dedicated temperature research：PENDING**

## Related machine-readable contract

`device_model_fields.outdoor_temperature`、`operation_data_r3`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md) 與 [mhi-ac-ctrl-esp32.md](../sources/mhi-ac-ctrl-esp32.md)。
