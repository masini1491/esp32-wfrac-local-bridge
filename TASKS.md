# Unfinished work queue

## Hot — Add portable persistence boundary + ESP-IDF NVS compile-only adapter

**Goal：** 建立 software-only、typed、host-testable 的 persistence boundary，並提供 ESP-IDF NVS compile-only adapter，支援未來 Operator ID、per-device confirmed scheme cache 與 bounded time/checkpoint metadata；不執行真實 NVS／hardware runtime，也不把 persistence 變成泛用 credential store。

**User-authorized scope（2026-10-05）：**
- 使用者已明確要求執行既有 Cold Candidate `Portable persistence boundary + ESP-IDF NVS compile-only adapter`。
- 這次授權包含 portable contract、fake/in-memory test implementation、ESP-IDF NVS adapter source、host tests、C6/C3 compile/link integration，以及為消除 current NVS prohibition / security-status drift所需的最低充分 governance/docs reconciliation。
- 不授權真實 NVS 寫入、flash／hardware execution、Wi-Fi／WF-RAC network、registration mutation、Matter runtime或 production deployment。

**Current authority / semantics to preserve：**
- Operator ID是敏感、opaque application identity；不得與 MAC / Device ID / Aircon ID合併。Self-managed / mirror runtime selection與 registration lifecycle仍不在本 Stage決定。
- `wfrac_transport::SchemeCache` 目前只有 `unknown / http_confirmed / https_confirmed`。Persistence只可保存 confirmed state；不得把 transient failure、fallback decision或 speculative scheme持久化成 confirmed truth。
- `wfrac_api::TimeQuality` 為 `unavailable / estimated / trusted`。Persisted checkpoint不是 trusted current time；load只提供 caller可選擇採用的 degraded checkpoint metadata，不得自動把 quality升為 trusted，也不得自行 seed/adopt system clock。
- Operator ID / scheme / time consumers維持既有 owner；persistence layer不取得 registration、transport policy或 time-source decision authority。

**Architecture / scope：**
- 優先新增小型 portable persistence component/boundary（例如 `wfrac_persistence` 或等價 owner）與 platform-specific ESP-IDF adapter（例如 `wfrac_persistence_espidf`）。
- Dependency direction維持 `platform adapter → portable persistence contract`；portable core不得依賴 ESP-IDF / NVS types。
- 使用 typed records / operations，不建立 unrestricted arbitrary key-value public API。
- 至少定義並 deterministic host-test：
  - Operator ID load/store/erase contract with bounded length/opaque bytes or UTF-8 semantics consistent with current API owner；
  - per-device scheme load/store/erase contract，僅 `http_confirmed / https_confirmed` 可持久化，unknown等價 absent/clear semantics需明確；
  - time checkpoint load/store/erase contract，至少含 64-bit Unix-seconds checkpoint與必要 version/validity metadata；consumer必須能分辨 absent / incompatible / corrupt / storage failure。
- Schema/version semantics需明確：future incompatible schema不得被默默當成 valid current record。
- Failure semantics需 bounded且可區分，例如 invalid argument、not found、incompatible/corrupt、storage failure；不得把所有 failure吞成「沒有資料」。
- Caller-owned buffers / bounded storage優先；不要為此建立 unbounded heap/string ownership。
- Fake/in-memory implementation只供 deterministic host tests，不宣稱 NVS durability。
- ESP-IDF NVS adapter可以包含實際 NVS API call path供未來 runtime使用，但本 Stage只做 source + compile/link validation；CI / 本次 execution不得呼叫真實 NVS runtime。
- NVS key/namespace layout需 bounded、versioned且避免直接把敏感值本身當 key。Per-device cache key需使用 caller提供的 opaque stable key/identifier boundary，不在本 Stage重新定義 MAC / Device ID / Aircon ID哪一個是 canonical persistence identity。
- 不自動把 loaded values套進 RuntimeOrchestrator、SchemePolicy、ApiClientContext或 EspSystemTimeProvider；integration/adoption由 future runtime owner explicit決定。

**Security boundary：**
- Operator ID仍視為 sensitive。Tests、fixtures、docs、logs只可使用 synthetic values；不得提交或輸出真實 Operator ID / Wi-Fi / Matter secrets。
- Current project尚未完成 encrypted NVS / Flash Encryption production hardening。本 Stage不得宣稱 NVS-stored Operator ID具 at-rest confidentiality，也不得因此把 production secret persistence視為安全完成。
- Adapter / errors / tests不得 log stored secret payload。
- No credential migration/import/export UI、no WebUI persistence、no provisioning workflow。

**Governance reconciliation：**
- Current `AGENTS.md` generic禁止 NVS。最小修正為：允許本 Stage的 **portable persistence contract + ESP-IDF NVS compile-only adapter + host/compile validation**；actual NVS runtime/hardware execution、production credential persistence與未授權相鄰 scope仍禁止。不得寫成 TASKS自行產生 authority。
- `docs/security.md` 必要時明確記錄：compile-ready NVS adapter ≠ encrypted storage / production secret-at-rest approval。
- `docs/architecture.md`、`docs/roadmap.md`、`README.md`只在 current truth / ownership / derived metrics實際受影響時做最低必要更新。
- Reference topic只有在 persistence形成獨立 retrieval intent且 existing identity/time/TLS topics不足時才新增；不要重複既有 authority內容。

**Validation / completion gate：**
- Deterministic host tests覆蓋 typed record round-trip、erase/not-found、schema/version mismatch、corrupt/invalid input、scheme confirmed-only invariant、time checkpoint never auto-trusted，以及 secret non-logging contract可離線驗證的部分。
- Portable public headers不洩漏 `nvs_handle_t`、`esp_err_t`或其他 ESP-IDF types。
- ESP-IDF adapter在 ESP-IDF v6.0.2 下對 ESP32-C6與ESP32-C3 compile/link PASS；不執行 NVS runtime。
- Existing host/software CI保持 green；materially affected existing six-cell ESP-IDF compile/link gate保持 green。
- Library readiness = `PRESERVED`。
- `git diff --check` PASS。
- README physical-line metrics若受影響，從 final tracked content重算。
- On success，`TASKS.md` 回到 minimal `EMPTY`，commit/push，perform canonical remote read-back，並回報 exact persisted surfaces、security limitations、remaining runtime integration、validation evidence與 result SHA。

**Hard exclusions：**
- No real NVS read/write execution in CI or this Stage；no hardware / flash / serial / Wi-Fi / HTTP/TLS / WF-RAC network execution。
- No actual registration/deregistration、Operator ID generation、self-managed/mirror selection或 account lifecycle。
- No automatic system-clock seed/adopt from persisted checkpoint；checkpoint不得被標成 trusted current time。
- No generic secret vault、encrypted-NVS implementation、Secure Boot、Flash Encryption、Matter fabric persistence或 production provisioning。
- Do not execute any remaining Cold Candidate。
