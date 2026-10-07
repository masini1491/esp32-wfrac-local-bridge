# Unfinished work queue

## Hot — Tighten 0x1E/0x1F response-no-value semantics

**Goal：** 對 WF-RAC operation-data `0x1E` / `0x1F` 做 software-only semantic guard tightening：保留「known query codes / confirmed responses」與 candidate semantic labels，同時把目前觀察到的 `FF FF FF` 明確建模為 response-no-value，而不是 universally unsupported、zero value 或已建立的 run-hours / fan-speed physical value。

**User-authorized scope（2026-10-07）：**
- 使用者已明確要求執行下一個 residual semantic-tightening tranche。
- 授權僅涵蓋 `0x1E` / `0x1F` classifier/spec/docs/tests 的 bounded semantic guard 與必要 host/CI regression。
- 不授權 `0x0D/0x21/0x32/0x34/0x35`、vane/3D Auto/advanced fixed-state、Device Model/Matter/runtime consumer integration、hardware/network 或任何 operation-data write。

**Pinned evidence / evidence level：**
- Primary WF-RAC authority：`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`（MIT）。
- All listed operation-data codes, including `0x1E` / `0x1F`, were requested over the WF-RAC HTTP path and answered.
- Observed response for both was `FF FF FF` => no usable value.
- Upstream notes that MHI-AC-Ctrl requests both codes twice using a DB6 unit selector which the WF-RAC bridge does not expose; “unit not selectable, so no value” is the likely explanation, but it is explicitly untested/inferred.
- Therefore the project must preserve:
  - visibility = `CONFIRMED_QUERY_RESPONSE_NO_VALUE`;
  - semantic = candidate run hours / candidate fan speed only;
  - cause of no-value = UNKNOWN / inferred hypothesis only;
  - universally unsupported = not established;
  - local validation = `HARDWARE_TEST_PENDING`.

**Scope / architecture：**
- Keep `0x1E` and `0x1F` as known safe query codes.
- Preserve complete raw `[code, OP1, OP2, OP3]`.
- Keep semantic kinds `run_hours_candidate` and `fan_speed_candidate`.
- Keep `scope=unknown`; do not infer per-indoor/shared-outdoor scope.
- Keep `value_quality=no_usable_value_established`.
- Explicitly establish negative guards:
  - `FF FF FF` means only “response present, no usable value established” in current evidence.
  - it does **not** prove the code is unsupported by WF-RAC.
  - it does **not** establish zero run hours / zero fan speed.
  - it does **not** establish a sentinel value that is universal across firmware/models.
  - DB6 selector limitation remains an untested/inferred explanation, not canonical cause.
- Do not add typed physical-value decoder, units, selector emulation, retry-by-selector, Device Model/Matter/runtime integration, or write behavior.

**Evidence/status reconciliation：**
- Update `docs/references/topics/operation-data.md` minimally to record the response-no-value and cause-unknown boundary.
- Update `specs/wf-rac-contracts.json` minimally so both entries explicitly preserve:
  - `CONFIRMED_QUERY_RESPONSE_NO_VALUE`;
  - exact observed response shape `FF FF FF`;
  - candidate semantic only;
  - no usable value established;
  - universally unsupported = false / not established;
  - no-value cause = UNKNOWN, with DB6 selector limitation only as inferred hypothesis;
  - `HARDWARE_TEST_PENDING`.
- Update validator/tests only as needed to make these guards regression-enforced.
- README/roadmap only if materially necessary; do not inflate evidence/status.

**Validation / completion gate：**
- Deterministic host tests must cover at least:
  - `0x1E` and `0x1F` classifier identity remains unchanged;
  - full raw preservation, including exact `FF FF FF` response shape;
  - scope remains `unknown`;
  - quality remains `no_usable_value_established`;
  - `code_known=true`, `unit_relation_known=false`;
  - safe-query inclusion remains unchanged;
  - no typed value/unit/selector-derived decoder API is introduced;
  - contract validator rejects promotion to universally unsupported, zero-valued semantics, confirmed selector cause, inferred scope, or `CONFIRMED_LOCAL`;
  - nearby `0x7C` and `0x0D/0x21/0x32/0x34/0x35` classifications remain unchanged.
- Existing host/protocol regression remains green.
- Materially affected C6/C3 compile/link gates remain green; if no platform/production compile surface changes, document why they are not materially affected rather than running them ceremonially.
- Python syntax / contract validator / validator tests / `git diff --check` PASS as applicable.
- Library readiness = `PRESERVED`.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back; final report must state that response-no-value is distinct from unsupported and that DB6-selector cause remains inferred/unvalidated.

**Hard exclusions：**
- No run-hours or fan-speed physical decoder.
- No `FF FF FF` => zero-value interpretation.
- No universally-unsupported claim.
- No confirmed DB6/root-cause promotion.
- No scope inference.
- No `CONFIRMED_LOCAL` promotion.
- No Device Model / Matter / runtime consumer integration.
- No hardware/network validation.
- Do not execute Cold items.
