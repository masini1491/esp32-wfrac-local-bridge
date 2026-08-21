# 硬體基準

| 角色 | 目標 |
|---|---|
| Primary | ESP32-C6 |
| Compatibility | ESP32-C3 |
| 候選開發板 | ESP32-C3 Super Mini、ESP32-C6 Super Mini、nanoESP32-C6 N4/N8/N16 |

N16 只代表較多 Flash，不代表較多 SRAM 或 CPU。架構不得綁死 16 MB；獨立 WF-RAC 專案不應要求 N16 才能運作。硬體 protocol/runtime validation：`HARDWARE_TEST_PENDING`。
