# Capability / model detection

## Scope

R4 的 ModelNrRaw、wire-layout、official-app capability 與 optional feature gating。

## Authoritative project conclusions

- ModelNrRaw 來源是 RECEIVE `state[0] & 0x7F`，必須保留 raw。Raw 0/1/2/3/64 對應 `separate_2021`、`global_2022`、`high_end_for_japanese_2023`、`zt_2025`、`fdt_2023`；official-app unknown fallback 是 separate_2021，但 project policy 是 conservative UNKNOWN。
- Raw 3 capability `zt_2025` 不可被 wire-layout group 2 覆蓋。Wire compatibility、app group、retail model 與 local observation 分離。
- 17 app flags 已記錄於 contract；flag 不是 wire presence proof，也不等於 universal capability。Raw 1 的 `operation_data=false` 不得禁止 R3 query。
- Optional gates：LR vane／Entrust／vacant／Home Leave／LED／energy／preset range 各自需 protocol evidence、app hint 與 local validation；self-clean flag 不建立 remote start/status/control。
- Known protocol modes 是 auto/cool/heat/fan/dry，但 per-model support UNKNOWN。Firmware metadata 僅作 compatibility／quirk context；retail series／size 不可由 wire 推導。Multi-split data scope 不等於 capability identity。

## High-risk invariants

Unknown raw 不得套用 fallback 後 optimistic enable optional writable features；不得建立無 evidence 的 model mode table 或 Matter mapping。

## Known unknowns / pending validation

每個 user-device raw、optional feature、per-model modes、setpoint range、LED／autoHeating、retail model 與 hardware differences：`HARDWARE_TEST_PENDING`。

## Related machine-readable contract

`capability_detection_r4`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md)。
