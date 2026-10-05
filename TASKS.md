# Unfinished work queue

## Hot — Add canonical operation-data ambient / energy decoders

**Goal：** 從既有 broad `Advanced protocol / diagnostic semantics` Candidate切出第一個 bounded、software-only read-side tranche：為 WF-RAC operation-data 建立 canonical `0x80` indoor/outdoor ambient lookup decoder與 `0x94` current-run energy exact decoder；永遠保留 raw segment，且不把其餘 diagnostic formulas／UNKNOWN semantics一起升級。

**User-authorized scope（2026-10-05）：**
- 使用者已明確要求執行下一個 Cold work；本 Stage由既有 `Advanced protocol / diagnostic semantics` 分解而來。
- 授權只涵蓋 portable protocol read-side decoder、host tests、必要 machine/docs reconciliation與既有 compile-smoke/CI整合。
- 不授權 hardware/network、operation-data write、vane/3D Auto control、Matter runtime或未知 diagnostic semantic promotion。

**Pinned evidence / provenance：**
- Primary WF-RAC authority維持 `blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
- Exact pinned source確認：
  - `0x80 / OP1=0x10` = outdoor ambient，使用 256-entry `outdoorTempList[OP2]`；
  - `0x80 / OP1=0x20` = indoor ambient，使用 256-entry `indoorTempList[OP2]`；
  - lookup值為 canonical WF-RAC network representation，與 direct CNS formula不同；
  - `0x94 / OP1=0x10` = per-run energy，`(OP3 << 8 | OP2) × 0.25 kWh`，不是 lifetime counter。
- 若需要引入兩張 lookup table，只可在 MIT license/provenance已明確記錄下，以 project-normalized numeric data（例如 integer deci-Celsius）最低充分引入；不得複製 upstream surrounding implementation／comments形成 mechanical port。
- 不使用 upstream newer main新增的 semantics覆蓋 project pin；若 current project authority與 pinned source衝突，STOP或保持較保守語義。

**Scope / architecture：**
- 延伸既有 `wfrac_protocol::OperationData` / operation-data owner，不另造第二套 protocol parser。
- 提供 deterministic、allocation-free／bounded read-side value decode boundary。具體 API shape可由 implementation選最低充分形式，但至少需：
  - 對 canonical `0x80` outdoor/indoor ambient輸出 exact **deci-Celsius integer**（或等價無浮點 lossless representation）；
  - 對 canonical `0x94` current-run energy輸出 exact **quarter-kWh count**與／或等價 exact integer Wh representation；不得用 float作唯一 canonical value；
  - 對 selector／semantic不匹配、UNKNOWN、unsupported、invalid argument提供明確 no-value/error disposition；
  - raw `[code,op1,op2,op3]`仍可完整取得，不因成功 physical decode被丟棄。
- Ambient decoder必須使用 network lookup table；禁止套用 direct CNS `(raw-61)/4` 或 `(raw-94)*0.25`。
- Energy明確標示 current-run/per-indoor diagnostic counter；decoder不累加 lifetime energy、不推定 reset fault、不跨 unit合併。
- 既有 `decode_operation_segment` 的 UNKNOWN/raw-only classification不得因本 Stage樂觀擴張：
  - `0x11` frequency、`0x90` current、`0x13` EEV、`0x81/0x87` coil curves、`0x82`、`0x85`、`0xB1`、`0x7C`、`0x1E/0x1F`、`0x0D/0x21/0x32/0x34/0x35`、`0xF8` 保持 current project quality/UNKNOWN boundary，除非在本 Stage發現既有 source/code inconsistency需要 fail-closed correction；不得順手 admission新 physical formula。
- 不把 App capability flags當 protocol-value presence proof；不因 capability group自動enable decoder。
- 不把 decoded diagnostics直接塞進 Device Model / Matter adapter；consumer integration另行 admission。

**Evidence-status reconciliation：**
- `docs/references/topics/operation-data.md` 與 `temperature-conversion.md` 只更新 implementation status／exact admitted decoder boundary，不改 local hardware evidence level；仍是 upstream-confirmed + hardware pending。
- `specs/wf-rac-contracts.json` 只在 machine-readable implementation/status truth需要時做最低充分更新；不得把 software decoder標成 `CONFIRMED_LOCAL`。
- Roadmap/README只在 current status或 derived metrics實際受影響時更新。
- 若 numerical table provenance需要 focused note，優先放既有 temperature/operation-data topic；不為同一 intent另建重複 topic。

**Validation / completion gate：**
- Deterministic host tests至少覆蓋：
  - 兩張256-entry ambient table的 size / representative endpoints / repeated-clamped low-end / representative midpoints，並對所有 256 raw values做 bounded lookup；
  - indoor/outdoor selector不可互換，未知 selector不得造值；
  - network lookup結果不等於誤套 direct CNS formula的 regression cases；
  - `0x94` 16-bit little value composition（OP2 low, OP3 high）、0、代表值、最大值，以及 exact quarter-kWh/Wh conversion；
  - `0x94` 非 canonical selector時不得誤解成 confirmed energy；
  - raw preservation與 existing UNKNOWN diagnostics semantics不退化。
- Existing protocol/host regression保持 green。
- Materially affected ESP-IDF C6/C3 compile/link gates保持 green；不執行 firmware/network。
- Library readiness = `PRESERVED`。
- `git diff --check` PASS；README metrics若受影響從 final tracked content重算。
- On success，`TASKS.md` 回到 minimal `EMPTY`，commit/push，canonical remote read-back；報告 exact decoder scope、remaining advanced UNKNOWN/residual Cold、validation與 result SHA。

**Hard exclusions：**
- No operation-data write (`OP1=0`)、exploratory control、real HTTP/WF-RAC/hardware。
- No vane / horizontal / 3D Auto write semantics。
- No `0x81/0x87` thermistor curve implementation、`0x85` discharge physical conversion、`0x11/0x90` physical units或其他 inferred formula admission in this tranche。
- No Device Model / Matter diagnostic integration。
- Do not execute the hardware Cold Candidate。
