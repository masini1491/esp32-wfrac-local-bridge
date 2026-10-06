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

## Software decoder boundary

`decode_ambient_temperature()` 現只對 `0x80/OP1=0x10` outdoor 與 `0x80/OP1=0x20` indoor 做完整256-entry network lookup，回傳 exact integer deci-Celsius、scope及完整raw。`decode_current_run_energy()` 現只接受 `0x94/OP1=0x10`，以OP2 low／OP3 high組成uint16 quarter-kWh count，exact Wh=count×250；不是 lifetime counter，不累加或推定reset fault。Unsupported selector/code不造physical value，raw仍保留，metadata reset；null output是invalid argument。

上述canonical classifier quality為 `canonical_ambient_lookup`／`exact_current_run_energy`；非canonical `0x94` selector為unknown。Canonical scope與query/write邊界不變，不接入Device Model／Matter。證據仍為 `CONFIRMED_UPSTREAM`＋software host validation；local hardware保持 `HARDWARE_TEST_PENDING`。

## Inferred outdoor diagnostic candidates

`decode_inferred_compressor_frequency()` 僅接受 `0x11/OP1=0x10..0x1f`，回傳 exact integer candidate deci-Hz=`(OP1-0x10)*256+OP2`；拒絕其他high bits，不以mask接受未建立的selector。`decode_inferred_operating_current()` 僅接受 `0x90/OP1=0x10`，以numerator=`OP2*14`、denominator=51保存exact rational amperes candidate；不推導power/COP/energy。

`decode_inferred_discharge_temperature()` 僅接受 `0x85/OP1=0x10` 且 `OP2>=0x12`，以half-degree count=`OP2+64`保存candidate（`OP2/2+32°C`）；低端回 `out_of_established_range`，沒有30°C clamp或fabricated32°C。Current/discharge的exact0x10是保守outdoor response-shape admission，不是universal OP1 marker。

成功的value／classifier quality為 `inferred_diagnostic_candidate`，三者皆shared outdoor；typed names明示INFERRED。失敗時non-null output reset，完整raw仍保留，candidate metadata無效。OP3保留但不參與公式。Pinned `docs/wf-rac-module-reference.md` §5.4與 `rac_parser.py::_apply_service_data_segment()` 是WF-RAC provenance；兩個operating points不是calibration，secondary MHI-AC-Ctrl只是CNS/SPI cross-reference。Exact software arithmetic不把physical relation升級成canonical/calibrated或 `CONFIRMED_LOCAL`；local仍 `HARDWARE_TEST_PENDING`。

`0x13` typed read-side decoder只回傳 per-indoor EEV-related WF-RAC network OP2 byte，並保留完整raw segment；它不是 OP2/255 percentage、校準閥門開度、full-open count或 direct CNS 16-bit counter。這只建立software decoder，不改 classifier、不接入consumer，也沒有operation write；local hardware仍 `HARDWARE_TEST_PENDING`。`0x81/0x87`、`0x82`、`0xB1`、`0x7C`與其餘residual classifications不變。

## High-risk invariants

App capability flags 不等於 protocol presence；firmware constant zero 不等於 unsupported。Temperature 詳細 conversion 見 [temperature-conversion.md](temperature-conversion.md)。

## Known unknowns / pending validation

Unknown code semantics、no-value cause、`0x7C` non-zero meaning、`0x13` scale、firmware matrix 與所有 local fields：`HARDWARE_TEST_PENDING`。

## Related machine-readable contract

`operation_data_r3`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md) 與 [mhi-ac-ctrl-esp32.md](../sources/mhi-ac-ctrl-esp32.md)。
