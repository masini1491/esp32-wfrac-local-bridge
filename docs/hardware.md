# Hardware baseline

| Role | Target |
|---|---|
| Primary | ESP32-C6 |
| Compatibility | ESP32-C3 |
| Candidate boards | ESP32-C3 Super Mini、ESP32-C6 Super Mini、nanoESP32-C6 N4/N8/N16 |

N16 只代表較多 Flash，不代表較多 SRAM 或 CPU。架構不得綁死 16 MB；standalone WF-RAC project 不應要求 N16 才能運作。硬體 protocol/runtime validation：`HARDWARE_TEST_PENDING`。
