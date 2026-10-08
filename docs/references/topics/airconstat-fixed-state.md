# AirconStat fixed state

## Scope

R2 的 COMMAND／RECEIVE framing、18-byte state、set-bit 與 fixed-state semantics；operation-data matrix 另見 [operation-data.md](operation-data.md)。

## Authoritative project conclusions

- Base64 AirconStat 由 COMMAND 與 RECEIVE 兩個獨立 block 組成；每個有 18-byte state、1-byte segment count、N×4-byte segments、CRC16-CCITT。RECEIVE offset 由 COMMAND segment count 推導。
- RECEIVE raw state 不等於 COMMAND raw state；network mask 會遮蔽 command set bits，不能由 RECEIVE 重建 COMMAND。portable framing／codec 與 selective command encoder 已實作；control request composition 另見 [request-composition.md](request-composition.md)。
- Fixed map：byte 0 model number；1 observed `0x04` but UNKNOWN；2 power/mode/vertical swing；3 fan/vertical vane；4 target；5 external room temperature injection，COMMAND 安全預設 `0xFF`；6 error read-only；7 echo；8 CoolHotJudge／operation-data；9 per-indoor compressor demand；10 vacant；11/12 horizontal／3D Auto；13/14/16/17 UNKNOWN safe default 0；15 self-clean bit unreliable。
- Vertical vane software codec（`CONFIRMED_UPSTREAM`）：RECEIVE byte 2 upper bits `01` (`& 0xC0 == 0x40`) indicates up/down swing; in that state byte 3 position bits are ignored. With byte 2 upper bits `00`, byte 3 upper nibble `0..3` maps to fixed positions 1..4. Other byte-2 upper-bit combinations and out-of-range position nibbles remain unknown. COMMAND selection is a paired byte-2/byte-3 encoding: swing `(0xC0,0x80)`, fixed positions 1..4 `(0x80,0x80/0x90/0xA0/0xB0)`; other fields remain at the safe command default. This establishes protocol representation only; model capability, actual actuation and timing remain `HARDWARE_TEST_PENDING`.
- RECEIVE mode `auto/cool/heat/fan/dry = 00/08/10/0C/04`；COMMAND `20/28/30/2C/24`。Fan RECEIVE `07/00/01/02/06`，COMMAND `0F/08/09/0A/0E`。Prose／implementation mode conflict 保留為 `INTERNAL_REFERENCE_CONFLICT`。
- Safe no-op query 清 writable set bits、byte 5=`0xFF`、必要時 carry byte 8；selective delta/full re-encode 尚待驗證，非 universal full-write requirement。

## High-risk invariants

Unknown command bytes 不可複製 RECEIVE；self-clean 不代表可靠 remote control/status；AirconStat indoor demand、direct CNS DB13、operation-data `0x11` 必須分離，不得合併 `compressorOn`。

## Known unknowns / pending validation

Byte 1/13/14/16/17、CoolHotJudge、self-clean、external temperature write safety、model-specific fields 與 local hardware：`HARDWARE_TEST_PENDING`。

## Related machine-readable contract

`aircon_stat_r2`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md) 與 [mhi-ac-ctrl-esp32.md](../sources/mhi-ac-ctrl-esp32.md)。
