# 參考 repository

| Repository | Revision | License | 優先級／用途 |
|---|---|---|---|
| [JobDoesburg/homebridge-mhi-wfrac](https://github.com/JobDoesburg/homebridge-mhi-wfrac) | `3d649dd6c6adbf50d405bb60961ffe820eb6241b` | Apache-2.0 | PRIMARY；WF-RAC 本地 API 與目前整合行為的主要參考 |
| [edwinvdpol/homey-wfrac](https://github.com/edwinvdpol/homey-wfrac) | `bddacfff797cd82a5cbffbd10c319ca22bb5daae` | MIT | PRIMARY；歷史與目前的 protocol codec、state model、discovery、reliability 參考 |
| [espressif/esp-matter](https://github.com/espressif/esp-matter) | `1e913f65d7f40eb8617be46b195dcacc9a80c165` | Apache-2.0 | PRIMARY；官方 Matter／Room Air Conditioner 參考 |
| [hberntsen/mhi-ac-ctrl-esp32](https://github.com/hberntsen/mhi-ac-ctrl-esp32) | `cb00cd54525acea9fbafdfee178a657eb6667faa` | MIT | LOW/MEDIUM；只作 MHI 功能與硬體側背景參考 |

研究筆記必須保留 provenance。四個選定 reference 的 license 為 Apache-2.0 或 MIT，目前沒有已知 GPL blocker。Project License 仍為 **TBD**。不可直接複製、mechanical port 或 line-by-line translation 第三方 implementation；必須保留 provenance。`mhi-ac-ctrl-esp32` 只作 MHI 功能與硬體側背景參考，不是 WF-RAC HTTP/HTTPS protocol authority。
