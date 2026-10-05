# Unfinished work queue

## Hot — Add portable runtime orchestration skeleton

**Goal：** 在不碰 ESP32 實機／網路的前提下，建立 portable host-testable runtime orchestration skeleton，把既有 Device Model、request executor、SchemePolicy、post-write verification 與 time/timeout boundary 串成 bounded control loop contract。

**Scope：**
- 新增 portable runtime/orchestration boundary；不得依賴 ESP-IDF、FreeRTOS、Wi-Fi、NVS、Matter 或 firmware runtime。
- 以 FakeTransport／fake clock／host tests 驗證：
  - bounded command queue / serialization；
  - polling 與 command coordination；
  - timeout / retry decision；
  - availability transitions；
  - post-write verification lifecycle；
  - superseding desired state / pending command interaction。
- 不 hard-code 未有 evidence 的 real-device timing；poll interval、settle delay、retry interval等保持 policy/configurable/UNKNOWN，不把假設冒充 hardware fact。
- 優先重用既有 `wfrac_client`、Device Model、SchemePolicy、TimeProvider/post-write verifier contracts；不要複製 protocol/API logic。
- Library readiness 必須維持 PRESERVED。

**Hard exclusions：**
- No hardware, flashing, serial, Wi-Fi, HTTP/HTTPS, TLS, WF-RAC network request, NVS, Matter runtime or production deployment。
- 不實作 production scheduler/thread/task owner；本 Stage 只建立 portable deterministic orchestration core + host tests。
- 不執行任何 Cold Candidate。
- 不新增真實 credentials / device-specific evidence。

**Validation / completion gate：**
- 全部新 orchestration 行為可由 deterministic host tests驗證。
- Existing host/software CI保持 green。
- ESP-IDF C6/C3 compile/link gate若因 portable dependency wiring受影響，需保持 green；不得用 platform-specific workaround污染 portable core。
- Architecture/roadmap/README只在 current truth或 derived metrics受影響時做最低必要更新。
- `git diff --check` PASS。
- 完成後 `TASKS.md` 回到 `EMPTY`，commit/push，並做 canonical remote read-back。
