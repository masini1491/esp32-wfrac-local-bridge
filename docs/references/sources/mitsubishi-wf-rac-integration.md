# Mitsubishi WF-RAC Integration

- Repository：`blues-sechseck/Mitsubishi-WF-RAC-Integration`
- Revision：`928efd8637bcb346a4945c545ab76733eae6245c`
- License：MIT
- Priority：PRIMARY / HIGH
- Authority boundary：主要 WF-RAC protocol／API／AirconStat reference；不是 local hardware evidence。
- Important files / evidence areas：`docs/wf-rac-module-reference.md`、`wfrac/rac_parser.py`、`wfrac/device.py`、`wfrac/repository.py`、`wfrac/capabilities.py`、`wfrac/utils.py`、`tests/unit/live_captures.py`、`tests/unit/test_rac_parser.py`；issue #223 is supplementary evolving hardware/calibration discussion and issue #189 supplementary real-hardware TLS compatibility evidence, neither pinned protocol authority。
- Topics：API、R1–R7、partial timestamp、temperature、operation-data、TLS。
- Known conflicts：prose mode mapping vs code；`0x7C` stale parser comment vs newer documented hardware evidence；full-state wording vs selective set-bit evidence。Official-app-derived ambient lookup tables are network-path authority；direct CNS formulas remain cross-layer only。
- Do-not-assume：upstream evidence is not `CONFIRMED_LOCAL`；direct capability、app flags、hardware path 不自動等於 WF-RAC universal truth。
- Request-composition evidence adopted：`docs/wf-rac-module-reference.md` §3／§4.3 與 `wfrac/rac_parser.py` 的 `to_base64()`、`receive_to_bytes()`、`status_request_to_byte()`；採用 COMMAND + RECEIVE、independent CRC、official no-request sentinel、request RECEIVE ignored as control、receive-format projection 與 byte8 carry behavior。未 copy upstream implementation；JobDoesburg/homebridge-mhi-wfrac (`3d649dd6c6adbf50d405bb60961ffe820eb6241b`) 與 edwinvdpol/homey-wfrac (`bddacfff797cd82a5cbffbd10c319ca22bb5daae`) 為 command block + receive-format block corroboration。
- Supplemental `0x13` idle-context provenance（不取代 primary pin）：`blues-sechseck/Mitsubishi-WF-RAC-Integration@966cedaa731b4d28b648f6ab58a1b0a16714c3e3`（merged PR #399），`docs/wf-rac-module-reference.md` §5.4，引用 discussion #328；該新增四室內機 idle-context觀察標為 `[UR]`（user-reported），不是本地測量或 universal firmware rule。僅用於記錄 system-direction-dependent idle raw observation與其 negative semantic guards。
