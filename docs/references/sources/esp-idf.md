# ESP-IDF system time

- Repository：`espressif/esp-idf`
- Revision：`0276131402f5383545ce5fc74f71d548fe5f3ffc`
- License：Apache-2.0
- Priority：PRIMARY / OFFICIAL PLATFORM REFERENCE
- Authority boundary：ESP32／ESP-IDF system-time、RTC timer、POSIX time、SNTP、timezone 與 `time_t` behavior；不是 WF-RAC protocol authority 或 Matter mapping authority。
- Important reviewed file：`docs/en/api-reference/system/system_time.rst`
- Important R7 files：`docs/en/api-reference/protocols/esp_tls.rst`、`components/esp-tls/esp_tls.h`、`components/esp_http_client/include/esp_http_client.h`。
- Topics supported：R5 timestamp／time architecture 與 R7 TLS platform capabilities。
- Known limitations：platform behavior 不等於 local hardware validation，也不決定 WF-RAC registration semantics。
- Do-not-assume：ESP32 所有 reset 都失去時間、power loss 後仍知道 elapsed wall time、或 SNTP 是 system time prerequisite。
