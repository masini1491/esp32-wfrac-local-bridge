# Unfinished work queue

## Hot — Add continuous ESP-IDF C6/C3 compile/link CI gate

**Goal：** 建立 bounded、continuous GitHub Actions ESP-IDF v6.0.2 compile/link gate，持續驗證 ESP32-C6 primary 與 ESP32-C3 compatibility target 的 platform integration 及 diagnostic firmware buildability；只建立 compile/link evidence，不執行 firmware、hardware、Wi-Fi 或 WF-RAC network traffic。

**Promotion reconciliation：**
- Current software validation workflow `.github/workflows/validate-contracts.yml` 只涵蓋 Python / host C++ validation，沒有 ESP-IDF compile/link job。
- Current repository已存在 ESP-IDF build consumers：
  - `firmware/idf_compile_smoke` — platform/component integration compile consumer；
  - `firmware/wfrac_bootstrap_probe` — default-inert diagnostic read-only probe firmware。
- 兩個 firmware CMake roots都使用 repository `components`，並把 generated `sdkconfig` 放在 build directory boundary。
- Current project authority固定 ESP-IDF **v6.0.2**、ESP32-C6 primary、ESP32-C3 compatibility；local compile/link evidence已存在，但尚未形成 continuous remote CI gate。
- Existing host/software validation workflow remains canonical for host tests and must not be weakened or replaced。

**Required implementation：**
- Add the minimum independent GitHub Actions workflow needed for ESP-IDF compile/link validation; prefer keeping the existing host/software workflow intact rather than coupling its ordinary validation path to ESP-IDF provisioning。
- Pin/resolve ESP-IDF to **v6.0.2** through an official or otherwise current-authority-supported CI provisioning route; do not silently float to another ESP-IDF release。
- Use a bounded matrix or equivalent deterministic structure covering at minimum:
  - `esp32c6` × `firmware/idf_compile_smoke`
  - `esp32c3` × `firmware/idf_compile_smoke`
  - `esp32c6` × `firmware/wfrac_bootstrap_probe`
  - `esp32c3` × `firmware/wfrac_bootstrap_probe`
- Each matrix cell must perform compile/link only and produce failure attribution clear enough to distinguish target / firmware consumer / provisioning or build failure。
- Trigger continuously for relevant `main` push / pull-request validation. A bounded manual trigger may be added only if useful and low-maintenance。
- Keep provisioning/cache strategy bounded and reproducible. Cache is optional unless current evidence shows it materially improves cost without weakening correctness; do not add a custom dependency-management framework only for this Stage。
- If workflow addition changes repository physical-line metrics or another tracked derived fact, update only the directly affected README derived values in the same Stage so completion does not leave stale metrics。
- Update roadmap/README status only where needed to accurately distinguish local compile gate from **continuous remote CI compile/link gate**; avoid broad documentation rewrite。

**Hard exclusions：**
- No hardware execution, flashing, serial monitoring, Wi-Fi association, HTTP/HTTPS execution, TLS handshake, real WF-RAC request, credential/operator/certificate material, NVS, runtime polling/retry, Matter, or production deployment。
- Do not alter firmware/source/components merely to make CI green unless current build evidence proves a genuine pre-existing compile/link defect and the Stage is STOP/reconciled before expanding mutation scope。
- Do not weaken or remove existing host validation。
- Do not execute the remaining Cold Candidate `First real WF-RAC read-only probe / hardware validation`。
- Do not claim Hardware/Network PASS from compile/link CI。

**Expected writable surfaces：**
- new or existing `.github/workflows/*` only as required for the continuous ESP-IDF gate；
- `README.md` / `docs/roadmap.md` only for directly affected CI status / derived metrics；
- `TASKS.md` for completion bookkeeping。
- Any source/component/firmware-content mutation outside CI/config/docs requires STOP + fresh reconciliation rather than automatic scope expansion。

**Validation / completion gate：**
- Workflow syntax/configuration is structurally valid and uses ESP-IDF v6.0.2。
- All four target × firmware compile/link cells complete successfully on GitHub Actions for the delivered revision (or an equivalent exact-revision remote run that proves the same workflow/content)。
- Existing host/software validation remains green or otherwise has no regression attributable to this Stage。
- Failure reporting identifies target and firmware consumer; no runtime/hardware/network evidence is claimed。
- Final scoped diff contains only authorized workflow/config/docs/TASKS surfaces。
- `git diff --check` PASS。
- Any README physical-line metrics changed by this Stage are recomputed from final tracked content and internally consistent。
- On success, remove this Hot Stage, return `TASKS.md` to minimal `EMPTY`, commit/push, perform canonical remote read-back, and report exact workflow/run evidence, changed-file list, applicable Playbook revision, and final remote HEAD。
