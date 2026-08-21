# Reference repositories

| Repository | Revision | License | Priority / role |
|---|---|---|---|
| [JobDoesburg/homebridge-mhi-wfrac](https://github.com/JobDoesburg/homebridge-mhi-wfrac) | `3d649dd6c6adbf50d405bb60961ffe820eb6241b` | Apache-2.0 | PRIMARY；WF-RAC local API / current integration behavior |
| [edwinvdpol/homey-wfrac](https://github.com/edwinvdpol/homey-wfrac) | `bddacfff797cd82a5cbffbd10c319ca22bb5daae` | MIT | PRIMARY；historical/current protocol codec, state model, discovery, reliability |
| [espressif/esp-matter](https://github.com/espressif/esp-matter) | `1e913f65d7f40eb8617be46b195dcacc9a80c165` | Apache-2.0 | PRIMARY；official Matter / Room Air Conditioner reference |
| [hberntsen/mhi-ac-ctrl-esp32](https://github.com/hberntsen/mhi-ac-ctrl-esp32) | `cb00cd54525acea9fbafdfee178a657eb6667faa` | MIT | LOW/MEDIUM；MHI capability / hardware-side background only |

Research notes must preserve provenance. All four selected references are Apache-2.0/MIT; no known GPL blocker is recorded. Project License remains **TBD**. Do not directly port or copy implementations, mechanically translate, or line-by-line reproduce code; preserve the provenance chain, including JobDoesburg’s acknowledgement of edwinvdpol’s Homey implementation. `mhi-ac-ctrl-esp32` is not WF-RAC network protocol authority.
