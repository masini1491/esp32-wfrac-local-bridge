# 參考 repository

| Repository | Revision | License | 優先級／用途 |
|---|---|---|---|
| [blues-sechseck/Mitsubishi-WF-RAC-Integration](https://github.com/blues-sechseck/Mitsubishi-WF-RAC-Integration) | `928efd8637bcb346a4945c545ab76733eae6245c` | MIT | PRIMARY / HIGH；WF-RAC protocol、network API、AirconStat、firmware/app/hardware evidence、CNS/SPI 關係與 operation-data channel 的主要參考 |
| [JobDoesburg/homebridge-mhi-wfrac](https://github.com/JobDoesburg/homebridge-mhi-wfrac) | `3d649dd6c6adbf50d405bb60961ffe820eb6241b` | Apache-2.0 | PRIMARY；WF-RAC 本地 API 與目前整合行為的主要參考 |
| [edwinvdpol/homey-wfrac](https://github.com/edwinvdpol/homey-wfrac) | `bddacfff797cd82a5cbffbd10c319ca22bb5daae` | MIT | PRIMARY；歷史與目前的 protocol codec、state model、discovery、reliability 參考 |
| [espressif/esp-matter](https://github.com/espressif/esp-matter) | `1e913f65d7f40eb8617be46b195dcacc9a80c165` | Apache-2.0 | PRIMARY；官方 Matter／Room Air Conditioner 參考 |
| [hberntsen/mhi-ac-ctrl-esp32](https://github.com/hberntsen/mhi-ac-ctrl-esp32) | `cb00cd54525acea9fbafdfee178a657eb6667faa` | MIT | SECONDARY / MEDIUM-HIGH；MHI CNS/SPI protocol cross-reference，用於交叉驗證 AirconStat 底層 state/frame semantics、set-bit、operation-data address space 與 MHI capability；不是 WF-RAC HTTP/HTTPS API authority |

研究筆記必須保留 provenance。五個選定 reference 的 license 為 MIT、Apache-2.0 或 MIT，目前沒有已知 GPL blocker。Project License 仍為 **TBD**。不可直接複製、mechanical port 或 line-by-line translation 第三方 implementation；必須保留 provenance。`mhi-ac-ctrl-esp32` 不是 WF-RAC HTTP/HTTPS API authority，但其 MHI CNS/SPI protocol knowledge 可作為理解 AirconStat 底層 state/frame semantics 的 cross-reference。

新增 reference 的 upstream evidence tags 為 `[HW]`、`[FW]`、`[APP]`、`[EXT]`、`[INF]`。它們全部仍屬外部 reference evidence；`[HW]`、`[FW]`、`[APP]` 若來源明確，寫入本 project 時只能標為 `CONFIRMED_UPSTREAM`，不得標為 `CONFIRMED_LOCAL`。`[INF]` 若採用，標為 `INFERRED`。本 project 的 `CONFIRMED_LOCAL` 仍為 none。

新增 reference provenance：Original author `jeatheak`；current maintainer／repository owner `blues-sechseck`；copyright 2022 `jeatheak`、2026 `blues-sechseck`。
