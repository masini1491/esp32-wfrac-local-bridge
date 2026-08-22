# Operation-data

## Scope

R3 WF-RAC-visible matrix、raw segment preservation、query transaction 與 CNS/SPI visibility boundary。

## Authoritative project conclusions

- Variable segment 必須保存完整 `[code, op1, op2, op3]`；OP1 不是 universal `0x10` response marker，OP3 可能承載資料。
- Generic query 使用 `setAirconStat` no-op COMMAND block 加 query segment，response 必須由同一 transaction 解析；不得改用稍後 `getAirconStat`。Query command：set bits clear、byte 5=`0xFF`、OP1=`0xFF`，batch 只作 small/bounded 方向。
- Project full query request composition 使用最新 CRC-valid RECEIVE raw state 作 compatibility baseline，COMMAND 只 carry byte 8 bit `0x08`；RECEIVE 固定使用 no-request sentinel，且不複製 observation response segments。完整 request batch bound 為 project policy 的 1–3 個 codes，仍為 `HARDWARE_TEST_PENDING`。
- Matrix：unsolicited `0x80/0x10` outdoor temp、`0x80/0x20` indoor temp、`0x94` current-run energy（0.25 kWh/count）；query values `0x11` frequency、`0x90` current、`0x85` discharge、`0x13` EEV raw、`0x81/0x87` indoor coils、`0x82` outdoor coil、`0xB1` TDSH-related、`0x7C` unresolved；`0x1E/0x1F` no-value；`0x0D/0x21/0x32/0x34/0x35` unknown semantic；`0xF8` special app extension。
- `0x7C` 不可建模為 protection boolean；`0x13` 不可升級為 calibrated percentage。`0x0C`／`0x05` 只有 direct CNS evidence，WF-RAC HTTP visibility UNKNOWN，不是 confirmed unsupported。Generic operation-data write 維持 OUT_OF_SCOPE。
- Per-indoor：demand、EEV、indoor coils、energy；shared outdoor：frequency、current、discharge、outdoor coil。Scope 不直接轉成 Matter endpoint。

## High-risk invariants

App capability flags 不等於 protocol presence；firmware constant zero 不等於 unsupported。Temperature 詳細 conversion 見 [temperature-conversion.md](temperature-conversion.md)。

## Known unknowns / pending validation

Unknown code semantics、no-value cause、`0x7C` non-zero meaning、`0x13` scale、firmware matrix 與所有 local fields：`HARDWARE_TEST_PENDING`。

## Related machine-readable contract

`operation_data_r3`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md) 與 [mhi-ac-ctrl-esp32.md](../sources/mhi-ac-ctrl-esp32.md)。
