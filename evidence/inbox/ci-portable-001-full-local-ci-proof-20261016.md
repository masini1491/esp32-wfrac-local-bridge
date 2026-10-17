# CI-PORTABLE-001 — Local full-profile evidence review (2026-10-16)

**Review status:** STRUCTURAL EVIDENCE REVIEW PASS. The reviewed JSON is a user-exported summary of a real local WSL2/Docker run. This review independently verifies internal structure and consistency, **not** re-execution or authentication of raw logs. This is not GitHub required-check evidence, PR #13 acceptance, or Stage 3 Gate A promotion.

## Provenance and integrity

- Source evidence: user-uploaded `ci-portable-001-full-proof-c3815a0.json`.
- JSON SHA-256 (as uploaded): `0995797c8049b52f39d557b21414824cebc981a388c51e83312e69edadd7be49`.
- Tested commit: `c3815a0de1eeefb2b435c16654d9ab1856b410d5` (local-only candidate, unpushed at review time).
- Tested Git tree: `323605876fdce7d31da5c398aaeb9fc2cdd9a2f3`.
- Docker digest / image ID: `sha256:0d8c9773d48a327233f9c1d7c654ff0bcf133ae24503ea2e97a57cfe02b8cb67`, `linux/amd64`.
- ESP-IDF pinned SHA: `7101770dc6db2667b3c477cc31365dd1acd6db4e`.
- ESP-lwIP pinned SHA: `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.
- `status=PASS`, `full_profile=PASS`, `preflight.status=PASS`.
- All nine `sdk_integrity` status values are PASS, with matching pinned provenance.
- Reported GitHub required-check status: **UNVERIFIED**.
- Reviewed remote `main`: `6393c84b5a46e619449e8938b9816535500d58f7`.
- Read-only reviewed remote Workflow blob SHA-1 identities: `esp-idf-compile.yml=76d71f4d52b1dcc330fdd8ea8032fb8be9da7aaf`; `validate-contracts.yml=833acb8820205a45765ae75634b986976c0476cc`.

## Full-profile suite results

| Suite | Recorded steps | Status | Runner suite wall seconds |
|---|---:|---|---:|
| `host` | 17 | PASS | 77.69 |
| `a6b` | 3 | PASS | 34.72 |
| `a7b` | 2 | PASS | 34.02 |
| `esp32c6-smoke-default` | 4 | PASS | 151.88 |
| `esp32c6-probe-default` | 4 | PASS | 163.62 |
| `esp32c3-smoke-default` | 4 | PASS | 159.59 |
| `esp32c3-probe-default` | 4 | PASS | 156.57 |
| `esp32c6-probe-webui` | 4 | PASS | 150.92 |
| `esp32c3-probe-webui` | 4 | PASS | 143.78 |

**46/46 recorded workflow steps PASS.** Host 17/17; A6b 3/3 workflow steps; A7b 2/2 workflow steps; firmware 6 × 4/4. Summed *suite* wall durations: 1072.80 seconds (17.88 minutes). This is a sum of reported suite durations, **not** an independently observed overall elapsed runtime.

## Firmware / A5 coverage

- Exactly six distinct target/variant cells: ESP32-C3 and C6 × smoke default, probe default, probe WebUI.
- Each cell reports all four steps PASS: ESP-IDF provisioning, compile/link (no firmware execution), A5 generated firmware configuration, A5 isolated SDK callback-fence compile/link (no execution).
- Thus six A5 configuration checks and six isolated A5 compile/link checks are marked PASS (12/12 named A5 steps).
- Each step includes a 64-hex-digit `workflow_command_sha256`, a name and elapsed seconds. Step names were compared against the current relevant GitHub Workflow step labels; this review **did not recompute every hash from extracted YAML commands or independently inspect raw container step logs**.
- Firmware was compiled/linked but not flashed or executed on ESP32 hardware. A6b and A7b run semantics cannot be certified down to individual internal test assertions from this summarized JSON alone.

## Additional historical context and outstanding acceptance gates

- Prior local user terminal output confirmed a separate Host 17/17 PASS on commit `c3815a0de1eeefb2b435c16654d9ab1856b410d5`; the full-profile proof reports a second Host PASS in this full run.
- Codex previously reported local CI unit tests 50/50 and existing Python tests 76/76 PASS. These are separate reports, not embedded in this JSON.
- WSL2 + Docker offline full-profile works on the tested machine and pinned image. Windows PS7 bootstrap handoff, fresh-machine rebuild, native Linux portability, and the requested repeated `--check` idempotence were not independently demonstrated here.
- Missing raw `result.json`, per-suite `steps.json`, detailed logs and signed attestation in this uploaded summary limits independent verification of output content and individual cases. They may remain on the user's local machine; preserve them.
- No GitHub Actions execution, required check restoration, push, PR creation, remote `TASKS.md` mutation, PR #13 merge, hardware run, or new Stage 3 Gate A/B authority is implied.
- Remote Draft PR #13 remained OPEN/DRAFT/UNMERGED at review time; head `2ceed6ec803a849229f17c3932440e56a8076e89`.

**Disposition:** `CI-PORTABLE-001 LOCAL_FULL_PROFILE = USER-EVIDENCED PASS / SUMMARY REVIEWED`. Repository integration and independent required-check proof remain blocked pending separate approval and available GitHub Actions budget. Stage 3 Gate A stays PENDING/UNPROVEN; Gate B ON HOLD; Stage 3 sole Hot; Library readiness PRESERVED.