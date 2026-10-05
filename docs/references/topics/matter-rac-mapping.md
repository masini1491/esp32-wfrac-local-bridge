# Matter Room Air Conditioner mapping

## Authority and scope

`CONFIRMED_UPSTREAM`：`espressif/esp-matter@1e913f65d7f40eb8617be46b195dcacc9a80c165`（Apache-2.0）的 generated Room Air Conditioner header/source 定義 device type `0x0072`、version `3`，必要 Descriptor／Identify／On/Off／Thermostat，以及 On/Off `dead_front_behavior`。其固定 `espressif/connectedhomeip@efefc94fee39d8d1fbbc3c27b9d7fc9025095887` submodule 的 `src/app/zap-templates/zcl/data-model/chip/thermostat-cluster.xml` 是 SystemMode／nullable／setpoint surface 的直接 evidence；本 Stage 不更新 upstream pin。

Project-admitted contract 位於 [`wfrac_matter_adapter`](../../../components/wfrac_matter_adapter/include/wfrac_matter_adapter/rac_mapping.hpp)，只使用 Device Model observed state／CommandIntent，不使用 esp-matter／CHIP SDK，不建立 node／endpoint／commissioning。這不是完整 Matter endpoint conformance 或 hardware evidence。Provenance 見 [esp-matter source](../sources/esp-matter.md)；WF-RAC temperature 語意見 [temperature conversion](temperature-conversion.md)。

## Conservative mapping policy

Mapping 必須區分 upstream field semantics 與 project policy；尚無證據的 feature、timing、物理控制結果不得由 adapter 推定。完整 mapping、actual Matter runtime／commissioning 仍 pending。Fan、Auto、Dry 與其他 optional WF-RAC fields 未因 device type 存在而自動 admission。

以下為 project software policy，不是 `CONFIRMED_LOCAL` Matter interoperability／physical actuation evidence。Pinned Thermostat 有 Off／Auto／Cool／Heat／FanOnly／Dry enum，但 enum 存在不代表 WF-RAC capability 或組合 write 語意已 admission。

| Device Model surface | Admitted projection / write contract |
|---|---|
| observed power | 已 available 且 observed 有值時，on/off → On/Off bool；unknown → withhold。On/Off write → 單一 `SetPowerIntent`。 |
| observed HVAC mode | power off → SystemMode Off；power on + cool/heat → Cool/Heat；unknown → withhold；auto/fan/dry → unsupported。Off write → power off；Cool/Heat write 僅在 available observed power on 時 → `SetModeIntent`。不暗中合成 power+mode command。 |
| observed target temperature | cool → OccupiedCoolingSetpoint，heat → OccupiedHeatingSetpoint；只投影當前 mode 的 target，不把單一 WF target 複製成兩個 setpoints。對應 attribute write 僅在 observed mode 相同時 → `SetTargetTemperatureIntent`。Auto 雙 setpoint／deadband 不推定。 |
| observed controller room temperature | explicit project proxy → nullable LocalTemperature；保留 centi-Celsius integer，不等同校準後室內 ambient sensor／operation-data indoor/outdoor temperature。超出 Thermostat 下界的值 → null。 |
| unavailable / unknown availability / no observed value | non-nullable On/Off／SystemMode／setpoints → withhold，不是 wire NULL 或預設 off/zero；nullable LocalTemperature → null。availability 只附診斷 metadata，不新增 Matter attribute。 |
| fan | 所有 FanSetting → unsupported；不建立 FanControl／custom cluster，也不產生 Fan write intent。 |

所有 projection 僅從 authoritative `observed` 取得資料，不讀取 optimistic desired／pending。`withhold`／`unsupported`／`invalid_value` 的 value storage 不可發布；未來 SDK adapter owner 必須處理 non-nullable attribute 初始化與 attribute write admission，不能把此 partial DTO 當完整 endpoint。

溫度沿用 Device Model centi-Celsius，恆等轉換且不 round／clamp／float。這個 DTO 邊界接受 `-27315..32767`，不是 WF-RAC 可控制溫度範圍；WF protocol range／lossless 0.5°C grid 仍由 existing protocol／RuntimeOrchestrator admission 驗證。例如 2325 可精確形成 intent，但不因此宣稱 WF write admissible 或 SDK write success。沒有套用 Matter SDK default setpoint limits 為裝置硬體能力。

Desired／pending／verification／availability lifecycle 仍由 Device Model／RuntimeOrchestrator 擁有；adapter 不改 DeviceState，不 dispatch、不 build API request、不 parse JSON／AirconStat、不管理 Operator ID／retry／discovery／socket。Mapping 成功不代表 command accepted、observed convergence 或 physical actuation。

## Remaining evidence boundary

Auto／fan-only／dry／fan speed／vane／3D Auto／outdoor temperature／energy／vacant-property／diagnostic／error-code mapping 仍 `UNKNOWN / unsupported`。Heating／Cooling feature advertisement、雙 setpoint lifecycle、完整 mandatory cluster implementation、actual SDK writes／Matter runtime／commissioning 與 interoperability 均 pending；不為填缺口建立 vendor cluster。

Tests：[`test_matter_mapping.cpp`](../../../tests/host_cpp/test_matter_mapping.cpp) 提供 deterministic software validation；ESP-IDF compile smoke 僅 compile/link，不執行 mapping runtime 或任何網路。
