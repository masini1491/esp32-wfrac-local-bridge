# Unfinished work queue

此檔是本 repository 的 current Hot coordination surface。只追蹤目前 executable／critical-path、已正式 admission 的工作；完成項目直接移除，Git history 才是完成紀錄。不得因看到本檔而自行執行未經使用者當次明確授權的 Stage。

本 repository 目前正在導入最新版 `masini1491/ai-development-playbook` 的 Hot／Cold coordination 與 AI Context routing。導入 Stage 完成前，舊有 Deferred items 暫時保留在本檔作 migration source；完成後應移至正式 Cold Registry，`TASKS.md` 收斂為最低充分 Hot-only／EMPTY state。

## Codex queue / launch rules

- 每次只執行使用者當次明確授權的 Stage；`TASKS.md` 本身不授權開始其他項目。
- 共通 repository identity、workspace / permission / credential capability、remote sync、Git safety、failure taxonomy / retry、validation / evidence、Completion Evidence Guard、AI Context loading、Model / Reasoning / Context / Agent / cost control與 reporting，以最新 `AGENTS.md` routing 到 `masini1491/ai-development-playbook` 的 canonical authority 為準；本檔不複製完整 common policy。
- 執行 Stage 前先安全同步最新 `origin/main`，重新讀最新 `AGENTS.md`、`TASKS.md` 與本 Stage 直接相關的最低必要 Playbook authority；若 Stage 已消失、scope materially 改變或 repository identity / Git state 不符，STOP。
- Repository reading 採最低充分 retrieval；禁止為熟悉專案預設 repo-wide 或 full-playbook 掃描。
- 不得因執行途中發現其他改善就自行跨 Stage、擴大治理 surface、建立額外 router/dossier/evidence framework，或順手修改 source/runtime/hardware。
- Stage 宣稱完成、更新 coordination state、commit / push 或移除本 Stage 前，必須依 Completion Evidence Guard 交叉確認 current HEAD、scoped diff、必要 validation 與 coordination state。

### Codex 短指令啟動格式

> 先安全同步最新 `origin/main`（僅允許 clean、fast-forward-only；異常即 STOP）。同步後讀根目錄 `AGENTS.md`、`TASKS.md`，並依 `AGENTS.md`／最新版 common playbook 只讀本 Stage 所需的最低必要 authority，完整執行 `TASKS.md` 的「Adopt latest AI Context / Hot-Cold coordination governance」；只執行該 Stage。完成前依 Completion Evidence Guard 交叉確認 canonical repository、coordination migration 與 scoped validation；成功後更新 coordination bookkeeping、commit / push（僅限本次 launch 已授權者）。

## Ready

### Adopt latest AI Context / Hot-Cold coordination governance

**推薦模型：** Luna  
**推理強度：** Low  
**推薦理由：** 這是 bounded governance / documentation restructuring；目標語意與來源規則已由最新版 Playbook 明確定義，不涉及 source/runtime/protocol implementation。重點是精確搬移 responsibility、避免複製 policy 與確認 retrieval-cost 淨改善。  
**是否值得先用較便宜模型做前置蒐證：** 否；Luna Low 已是最低充分設定。  
**Context 建議：** L0 → L2。先讀 project `AGENTS.md` / `TASKS.md`，再只讀 latest Playbook 的 `CHAT_INIT.md`、`AI_CONTEXT.md`、`REPOSITORY_EXECUTION.md` relevant sections；必要時才讀 `CHATGPT_WORKFLOW.md` 的 coordination/admission section。不要完整掃描 Playbook。  
**Execution mode 建議：** Normal / Agent 1。  

**Goal：** 將本 repository 的 AI-facing coordination / loading governance 對齊最新版 `masini1491/ai-development-playbook`，使 `TASKS.md` 回歸 Hot-only surface，新增明確 Cold Registry，並更新 project thin routing / ChatGPT write allowlist；降低 ordinary AI bootstrap 的不必要 Context，而不建立第二份 architecture/status/evidence database。

**Required changes：**

1. 更新 root `AGENTS.md` 的 common-playbook thin routing：
   - 明確納入 `CHAT_INIT.md` 作 new-session minimal bootstrap 概念；不得要求 full README / full Playbook scan。
   - 新增 `AI_CONTEXT.md` routing，涵蓋 AI readability、Context loading、Hot/Cold/Evidence/Historical、routing / retrieval cost。
   - 保留 project-specific governance / architecture / security / protocol authority 優先；不要把 Playbook common policy全文複製進 project `AGENTS.md`。
2. 在 project governance 明確 opt-in ChatGPT Coordination Write Allowlist：
   - `/TASKS.md` — Hot coordination。
   - `/BACKLOG.md` — Cold Registry。
   - 本 Stage **不要** opt-in `tasks/active/*`、`evidence/inbox/*` 或其他新 writable surface。
3. 建立 root `/BACKLOG.md` 作 Cold Registry，語意需符合 latest Playbook：
   - ordinary bootstrap 預設不載入；
   - 不具 execution authority；
   - Cold item 必須經 current authority/evidence reconciliation、promotion 到 Hot、並取得使用者明確 launch 才能執行；
   - AI-originated persistence 不提高 recommendation authority。
4. 將本檔目前三個舊 `Deferred` item **完整且不失真地遷移**到 `/BACKLOG.md`，保留 material goal / trigger / evidence / exclusions / completion gate，但可刪除只服務 Hot launch 的冗長重複 common-policy wording；不得改變 technical premise、不得把 item 自動升格為 committed execution obligation。
   - `Machine-readable TimeProvider implementation-state reconciliation`
   - `Continuous ESP-IDF C6/C3 compile/link CI gate`
   - `First real WF-RAC read-only probe / hardware validation`
   若 current evidence 足以區分 Candidate / Committed，可依 Playbook語意標記；若不足，保守保留既有 admission強度，不自行提高 authority。
5. migration 完成後，將 root `TASKS.md` 收斂為最低充分 Hot-only state；本 Stage 自身完成後移除。若沒有其他 Hot work，保留 persistent file但標明 `EMPTY`／等價明確語意，不保留舊 Deferred全文或 Completed changelog。
6. 檢查 README / docs / source 是否需要為 correctness 同 transaction 修改。若只是因 coordination 結構改變而產生 derived bookkeeping churn，依 Derived Metadata Write-Closure Gate **不要修改**；只有 existing project-facing instruction 會因此 materially 錯誤時才做最小必要 docs correction。

**Explicit exclusions：**

- 不建立 `docs/AI_PROJECT_MAP.md` 或其他 task-oriented project map；待 Hot/Cold/routing 導入後有實際 navigation evidence 再判斷 Independent Retrieval Intent Gate。
- 不建立 Hot task dossier framework。
- 不建立 evidence staging framework。
- 不修改 `components/`、`firmware/`、`specs/`、tests、tooling、CI、protocol、runtime、hardware behavior。
- 不執行任何 real WF-RAC、Wi-Fi、HTTP/HTTPS、TLS 或 hardware/network test。
- 不以固定 KB / token / line-count 作 universal readability gate。

**Validation / review gate：**

- `AGENTS.md` 的 project-specific authority不被 common Playbook覆蓋，且 routing 沒有形成第二份完整 common policy。
- `/BACKLOG.md` 明確是 Cold / default-not-loaded / non-executable surface，且三個舊 Deferred item 均可追溯、無 material information loss。
- `TASKS.md` 完成後只保存 Hot responsibility；若無 Hot task則為最低充分 EMPTY state。
- ChatGPT direct-write allowlist只擴到 `/TASKS.md` + `/BACKLOG.md`，沒有自我擴張到 docs/source/dossier/evidence path。
- normal new-session path可合理收斂為：`Project identity → minimal Playbook bootstrap/routing → project AGENTS → current Hot coordination → task-specific canonical source`；Cold Registry不成為 ordinary bootstrap必讀。
- 依 `AI_CONTEXT.md` Readability / Retrieval Cost Change Gate 檢查 always-on impact、routing depth、duplication、bounded-read、search noise、write closure與 net effect；結論應為 retrieval cost下降或至少沒有為形式增加 lookup。
- 執行最低充分 Markdown/link/path consistency check與 `git diff --check`；本 Stage 不要求 build/compile/test，除非修改內容意外觸及會影響既有 deterministic validation 的檔案，屆時先確認 scope是否合法。

**Completion gate：** `AGENTS.md` thin routing / allowlist、root `/BACKLOG.md` Cold Registry、三個 Deferred migration、root `TASKS.md` Hot-only/EMPTY lifecycle全部一致且 remote repository可驗證；沒有引入未授權新 surface或 technical scope change。成功後 commit / push並在 final report提供 commit SHA、changed files、migration摘要與 validation結果。

## Deferred — migration source only

> 下列內容在本 governance Stage 完成前保留作 canonical migration input；不得因位於本節自行 launch。完成 Stage 後應從 `TASKS.md` 移除並由 `/BACKLOG.md` 承接 Cold semantics。

### Machine-readable TimeProvider implementation-state reconciliation

**推薦模型：** Luna  
**推理強度：** Low  
**推薦理由：** current drift、直接 source 與 validator location 均已確認；工作重點是精確區分「portable TimeProvider boundary 已實作」與「production time acquisition/persistence/runtime 尚未完成」，屬 bounded contract/test maintenance。  
**是否值得先用較便宜模型做前置蒐證：** 否；Luna Low 已是最低充分設定。  
**Context 建議：** L0 → L2；只讀 `specs/wf-rac-contracts.json` 的 R5、`tools/validate_contracts.py` 對應 checker/tests、portable `time_provider.hpp`、ESP-IDF time adapter與 current roadmap/topic。  
**Execution mode 建議：** Normal / Agent 1。  
**Admission reason：** machine-readable contract 仍將 `future_time_provider.exact_runtime_enum` 標為 `PENDING` 且 `implementation=false`，validator 也強制此舊狀態，但 current source 已有 `TimeQuality { unavailable, estimated, trusted }`、`ITimeProvider` 與 ESP-IDF provider。這會使 validation PASS 無法代表 machine-readable implementation-state 與 current source 一致，屬 material contract/validation drift。  
**限制：** 不得把尚未存在的 trusted-time acquisition、persistence、SNTP/NTP dependency、power-loss recovery或 production runtime宣稱為已完成；只校正 machine-readable implementation-state vocabulary 與相應 validator/tests/docs consistency。  
**Completion gate：** contract 能明確表示 portable provider/time-quality boundary 已實作，同時保留 actual time acquisition/persistence/runtime 的未完成狀態；validator/tests 對新語意有 targeted coverage，既有 host validation PASS。成功後移除此項。

### Continuous ESP-IDF C6/C3 compile/link CI gate

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 涉及 GitHub Actions、ESP-IDF v6.0.2 toolchain provisioning/caching、matrix scope 與 validation cost，需要兼顧可重現性與 CI 成本，但不需要 Sol。  
**是否值得先用較便宜模型做前置蒐證：** 可；若只盤點 current workflow 與官方 ESP-IDF CI 安裝方式，可先用 Luna Low 唯讀蒐證，完成後 STOP 由使用者決定正式 implementation launch。  
**Context 建議：** L0 → L3；先讀 workflow、兩個 firmware CMake、adapter CMake 與 toolchain contract，不做 repo-wide loading。  
**Execution mode 建議：** Normal / Agent 1。  
**Admission reason：** current GitHub Actions 只覆蓋 machine contract、Python host tests 與 portable C++ host tests；ESP-IDF-specific transport/time adapters、`idf_compile_smoke`、`wfrac_bootstrap_probe` 與 C6/C3 portability 沒有 continuous CI regression gate。這是持續性的 platform/validation coverage，而非一次性小修。  
**限制：** 只建立 compile/link CI；default-inert，不得執行 hardware、Wi-Fi、HTTP/HTTPS、TLS handshake 或 real WF-RAC；不得把 credential/operator/cert material放入 workflow。CI 成本需 bounded，避免無必要 matrix/context expansion。  
**Completion gate：** current supported ESP-IDF baseline 下，至少能自動 compile/link platform integration 與 diagnostic firmware 的 ESP32-C6 primary、ESP32-C3 compatibility targets；failure phase 可區分 toolchain/config/source；既有 host validation保持 PASS。成功後移除此項。

### First real WF-RAC read-only probe / hardware validation

**推薦模型：** Terra  
**推理強度：** Medium  
**推薦理由：** 需要把實機 Wi-Fi/TLS/WF-RAC evidence 與既有 classifier/parser 語意整合判讀；屬 hardware-facing integration/debug，不適合 Luna 自行決策，也不需 Sol。  
**是否值得先用較便宜模型做前置蒐證：** 若只是確認 build/toolchain/Git state，可用 Luna Low；實際 probe evidence interpretation 仍由 Terra。  
**Context 建議：** L0 → L2；先讀 Git/AGENTS/TASKS/current evidence，再讀 diagnostic firmware、transport classifier、getDeviceInfo parser 與直接 caller/dependency；只有能明確說出缺少哪個答案時才升 L3，禁止預設 L4/L5。  
**Execution mode 建議：** Hardware evidence / Agent 1；不使用 Multi-Agent。  
**觸發條件：** #4H diagnostic harness compile-ready；當次測試 target/board/FQBN/device identity 明確；local endpoint/scheme/time seed/TLS trust inputs 明確；使用者明確授權本次 hardware/network execution。  
**PC pre-evidence（僅限 observed profile）：** 2026-09-02 的 Windows PC 已對兩台 `firmType=WF-RAC`、wireless `010`、MCU `131` 模組，以 explicit plain HTTP、port `51443` 完成 `getDeviceInfo` 與 `getAirconStat`（HTTP `200`、`result: 0`）。下一次 ESP32-C3/C6 probe 可將此作為 known-good read-only baseline：先以 explicit HTTP `:51443` 驗證 ESP-IDF diagnostic transport是否重現 `getDeviceInfo` HTTP `200` 與 `result: 0`。此 PC evidence不構成 ESP32-C3/C6 Hardware/Network PASS，也不建立通用 scheme、HTTPS 或 firmware 行為；相關 target evidence仍為 pending。詳見 `docs/references/sources/local-wfrac-pc-validation-2026-09-02.md`。  
**限制：** bounded、read-only、`getDeviceInfo` first；不得 insecure TLS、不得自動 downgrade、不得寫入控制命令。任何真實 credential/operator/cert material不得 commit、log、serial output 或 final report。Hardware/network execution authorization不自動包含 repository mutation、commit或 push authorization。  
**Completion gate：** 真實 ESP32 target 上的 bounded read-only `getDeviceInfo` probe 取得符合本 Stage evidence contract的 hardware/network結果；失敗時依 classifier / evidence分類而不猜 root cause。後續 `getAirconStat` 若仍必要，另經 current planning/admission決定，不自動執行。
