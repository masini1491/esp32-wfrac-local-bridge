# Control request composition

## Scope

Control request 的 raw AirconStat representation：single Device Model command intent、latest RECEIVE observation、COMMAND + RECEIVE block composition。這不是 Base64/API envelope、HTTP 或實際 request runtime。

## Adopted upstream conclusions

`CONFIRMED_UPSTREAM`：official app request 由獨立 CRC 的 COMMAND block 與 RECEIVE block 組成；沒有 variable operation 時，兩個 block 都使用 no-request sentinel `[01 FF FF FF FF]`。request RECEIVE half 不是第二個 control channel，使用 receive-format representation，而非複製 COMMAND bytes；採用 evidence 顯示 COMMAND byte 8 需 carry latest RECEIVE 的 bit `0x08`。

## Project safety policy

`PROJECT_POLICY / INFERRED`：正式 control request 必須有最新 CRC-valid RECEIVE observation。builder 複製其 18-byte raw state 作 compatibility baseline，只 patch 本次 intent 的 receive-side field，並拒絕 fabricated constant RECEIVE half 或缺少 baseline 的 request。此 policy 不是 universal WF-RAC protocol requirement。

## Operation-data query request composition

`PROJECT_POLICY / INFERRED`：full safe operation-data query request 已實作為 1–3 個既有 allowlisted generic query segments、最新 CRC-valid RECEIVE compatibility baseline、COMMAND byte 8 bit `0x08` carry，以及 RECEIVE no-request sentinel。COMMAND 只帶實際 query segments；RECEIVE 不複製 observation 的 response segments。此為 pure raw AirconStat composition，仍未建立 Base64/API envelope 或任何 request runtime。

## Command acceptance boundary

PC local write acceptance與後續 module-observed state不等於 indoor unit physical actuation confirmed。Google Assistant／Smart M-Air 偶發顯示控制完成但實體未執行，fresh read 後顯示可恢復正確現況；其 root cause 維持 `UNKNOWN`。見 [2026-09-02 PC local evidence](../sources/local-wfrac-pc-validation-2026-09-02.md)。

## Known unknowns

`CONFIRMED_LOCAL`：兩台 `WF-RAC / wireless 010 / MCU 131` profile module 的 PC local `setAirconStat` 已獲接受，後續 module-observed state 回報設定由 `24.0°C` 改為 `26.0°C`。這證實該 observed profile接受實際 control write並可回報 readback state，但不單獨驗證 physical HVAC actuation、本 project 的 composition policy、ESP-IDF transport execution或其他 model/firmware。後者仍為 `HARDWARE_TEST_PENDING`。見 [2026-09-02 PC local evidence](../sources/local-wfrac-pc-validation-2026-09-02.md)。

## Provenance pointers

Primary：`blues-sechseck/Mitsubishi-WF-RAC-Integration` at `928efd8637bcb346a4945c545ab76733eae6245c`，`docs/wf-rac-module-reference.md` §3／§4.3 與 `wfrac/rac_parser.py` 的 `to_base64()`、`receive_to_bytes()`、`status_request_to_byte()`。`JobDoesburg/homebridge-mhi-wfrac` at `3d649dd6c6adbf50d405bb60961ffe820eb6241b` 與 `edwinvdpol/homey-wfrac` at `bddacfff797cd82a5cbffbd10c319ca22bb5daae` corroborate command block + receive-format block。未複製 upstream implementation。
