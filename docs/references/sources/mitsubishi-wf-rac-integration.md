# Mitsubishi WF-RAC Integration

- Repository：`blues-sechseck/Mitsubishi-WF-RAC-Integration`
- Revision：`928efd8637bcb346a4945c545ab76733eae6245c`
- License：MIT
- Priority：PRIMARY / HIGH
- Authority boundary：主要 WF-RAC protocol／API／AirconStat reference；不是 local hardware evidence。
- Important files / evidence areas：`docs/wf-rac-module-reference.md`、`wfrac/rac_parser.py`、`wfrac/device.py`、`wfrac/repository.py`、`wfrac/capabilities.py`、`tests/unit/live_captures.py`。
- Topics：API、R1–R4、partial timestamp、temperature、operation-data、TLS。
- Known conflicts：prose mode mapping vs code；`0x7C` stale parser comment vs newer documented hardware evidence；full-state wording vs selective set-bit evidence；outdoor temperature conversion conflict。
- Do-not-assume：upstream evidence is not `CONFIRMED_LOCAL`；direct capability、app flags、hardware path 不自動等於 WF-RAC universal truth。
