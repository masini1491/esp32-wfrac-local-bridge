# Unfinished work queue

## Hot — Tighten unknown operation-data response-shape semantics

**Goal：** 對 WF-RAC operation-data `0x0D` / `0x21` / `0x32` / `0x34` / `0x35` 做 software-only semantic guard tightening：保存目前已觀察到的 raw response shapes 與 code-specific unknown semantic identity，同時禁止把單次 observed bytes 升成 physical meaning、通用 selector規則、single-byte value假設或 universal sentinel。

**User-authorized scope（2026-10-07）：**
- 使用者已明確要求執行下一個 residual semantic-tightening tranche。
- 授權僅涵蓋這五個 unknown-semantic codes 的 spec/docs/tests/validator bounded tightening 與必要 host/CI regression。
- 不授權 vane / horizontal vane / 3D Auto / advanced fixed-state、Device Model/Matter/runtime consumer integration、hardware/network、operation-data write 或其他 newly evidenced residual。

**Pinned evidence / observed response shapes：**
Primary WF-RAC authority：
`blues-sechseck/Mitsubishi-WF-RAC-Integration@928efd8637bcb346a4945c545ab76733eae6245c`.

All five codes were queried over the WF-RAC HTTP path and produced responses on the pinned observed unit:
- `0x0D → 00 FF FF`
- `0x21 → 10 FF FF`
- `0x32 → 2E 04 FF`
- `0x34 → 11 01 04`
- `0x35 → 10 00 FF`

Upstream explicitly notes:
- OP1 is a selector-like byte but is not universally fixed to `0x10`; `0x0D` and `0x32` already violate that pattern.
- `0x34` is the observed code that uses OP3, proving at least one operation-data response can carry multi-byte value content.
- Exact semantics for all five remain UNKNOWN.
- These observed shapes are hardware observations, not universal cross-firmware/model constants.

**Scope / architecture：**
- Keep all five as known safe query codes.
- Preserve complete raw `[code, OP1, OP2, OP3]`.
- Keep current semantic kinds:
  - `unknown_semantic_0d`
  - `unknown_semantic_21`
  - `unknown_semantic_32`
  - `unknown_semantic_34`
  - `unknown_semantic_35`
- Keep `scope=unknown`.
- Keep `value_quality=unknown_semantic`.
- Keep `unit_relation_known=false`.
- Do not add typed physical-value decoders or inferred units.
- Do not reinterpret OP1 as a universal status marker or selector with fixed semantics.
- Do not treat OP2 as the only value byte for all unknown codes.
- Explicitly preserve that `0x34` observed response uses OP3 and therefore must remain multi-byte-capable.
- Do not infer whether `0x34` is endian-specific, signed/unsigned, counter, temperature, state, flagset, or other value type.
- Do not interpret `FF` bytes in `0x0D/0x21/0x32/0x35` as a universal sentinel, unsupported marker, zero, missing unit, or fixed no-value rule.
- Do not infer cross-code semantic families merely because response shapes look similar.

**Evidence/status reconciliation：**
- Update `docs/references/topics/operation-data.md` minimally to record the five observed shapes and unknown-semantic boundary.
- Update `specs/wf-rac-contracts.json` minimally so each code records:
  - visibility = `CONFIRMED_QUERY_RESPONSE_UNKNOWN_SEMANTIC`;
  - exact observed response shape;
  - semantic/scope/value meaning = UNKNOWN;
  - full raw preservation;
  - observed shape is not a universal firmware/model sentinel/constant;
  - `HARDWARE_TEST_PENDING`.
- For `0x34`, explicitly record OP3 participation / multi-byte evidence and forbid single-byte-only interpretation.
- Update validator/tests only as needed to regression-enforce these guards.
- README/roadmap only if materially necessary; do not inflate status/evidence.

**Validation / completion gate：**
- Deterministic host tests must cover at least:
  - code-specific classifier identity for all five;
  - exact raw preservation for representative observed shapes;
  - `scope=unknown`, `value_quality=unknown_semantic`, `code_known=true`, `unit_relation_known=false`;
  - known-safe-query inclusion remains unchanged;
  - `0x34` OP3 is preserved and not discarded/ignored by a single-byte assumption;
  - no typed value/unit decoder API is introduced;
  - validator rejects semantic promotion, scope inference, universal OP1 rule, universal FF sentinel claims, single-byte-only `0x34`, or `CONFIRMED_LOCAL`;
  - nearby `0x7C`, `0x1E/0x1F`, `0xB1` classifications remain unchanged.
- Existing host/protocol regression remains green.
- Materially affected C6/C3 compile/link gates remain green; if no platform/production compile surface changes, document why they are not materially affected rather than running them ceremonially.
- Python syntax / contract validator / validator tests / `git diff --check` PASS as applicable.
- Library readiness = `PRESERVED`.
- On success, `TASKS.md` returns to minimal `EMPTY`, commit/push, canonical remote read-back.

**Hard exclusions：**
- No physical decoder or unit inference for any of the five codes.
- No universal OP1 selector/status rule.
- No `FF` universal sentinel/unsupported interpretation.
- No single-byte-only `0x34` interpretation; OP3 must remain meaningful raw evidence.
- No endian/signedness/value-type invention for `0x34`.
- No scope inference.
- No `CONFIRMED_LOCAL` promotion.
- No Device Model / Matter / runtime consumer integration.
- No hardware/network validation.
- Do not execute Cold items.
