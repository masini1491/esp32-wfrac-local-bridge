# MHI AC control ESP32

- Repository：`hberntsen/mhi-ac-ctrl-esp32`
- Revision：`cb00cd54525acea9fbafdfee178a657eb6667faa`
- License：MIT
- Priority：SECONDARY / MEDIUM-HIGH
- Authority boundary：direct CNS/SPI cross-reference；不是 WF-RAC HTTP/HTTPS API authority。
- Important files / evidence areas：DB0/DB1/DB2/DB3、set-bit architecture、DB16/17、operation-data address space、compressor separation、temperature conversion conflict。
- Topics：airconstat-fixed-state、operation-data、temperature-conversion。
- Known limitations / conflicts：rough formulas、community assumptions 與 WF-RAC representation差異需保留 evidence boundary；direct CNS temperature formulas must not decode WF-RAC network OP2。
- Do-not-assume：direct CNS/SPI capability 不證明 WF-RAC network-visible capability；不可 copy/port implementation。
