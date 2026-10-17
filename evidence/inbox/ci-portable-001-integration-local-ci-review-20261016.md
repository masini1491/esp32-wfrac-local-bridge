# CI-PORTABLE-001 — Integration full-profile evidence review

**Review recorded:** 2026-10-16 21:12 (Asia/Taipei)
**Repository:** `masini1491/esp32-wfrac-local-bridge`
**Candidate branch:** `chatgpt/ci-portable-001-integration-20261016`
**Candidate SHA:** `4a92b0bdccf9075cd64eaccec8f398b2649961f5`
**Tested Git tree:** `00dc010774c08a8d65c6855915028a23eb951f75`
**Uploaded summary SHA-256:** `2ee7a2ee81c1176c8ee1c5cdbc02df3bea8282f8b59986039a230436545bf51d`

## Evidence disposition

**USER-RUN LOCAL FULL PROFILE PASS / CHATGPT STRUCTURAL REVIEW PASS.**

The uploaded JSON is an exported summary from the user's actual WSL2 + Docker execution.
This review checked schema, counts, statuses, identity and provenance consistency;
it did not independently rerun Docker, authenticate raw logs or independently
recompute each Workflow command SHA-256 from raw step definitions. No GitHub
required-check or hardware/production PASS is implied.

- `status=PASS`, `full_profile=PASS`, preflight `PASS`.
- Exactly 9/9 suites PASS and 46/46 reported steps PASS.
- Six ESP32-C3/C6 firmware cells each report successful provisioning,
  compile/link, A5 generated-configuration, and A5 isolated SDK callback-fence
  compile/link steps: 12/12 named A5 checks PASS.
- All nine `sdk_integrity` statuses are PASS; each provenance row records
  ESP-IDF `7101770dc6db2667b3c477cc31365dd1acd6db4e` and
  esp-lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`.
- Docker image ID/digest `sha256:0d8c9773d48a327233f9c1d7c654ff0bcf133ae24503ea2e97a57cfe02b8cb67`,
  platform `linux/amd64`.
- The 46 recorded workflow step-name / command-hash pairs match the prior
  `c3815a0` summary, suite by suite; no workflow drift is indicated in
  these derived summaries.
- The sum of the nine *per-suite reported* elapsed times is
  **1049.84 seconds**. It is not
  an independently measured end-to-end runtime.

## Suites

| Suite | Steps | Outcome | Reported elapsed seconds |
|---|---:|---|---:|
| `host` | 17 | PASS | 66.71 |
| `a6b` | 3 | PASS | 35.22 |
| `a7b` | 2 | PASS | 34.90 |
| `esp32c6-smoke-default` | 4 | PASS | 158.73 |
| `esp32c6-probe-default` | 4 | PASS | 155.89 |
| `esp32c3-smoke-default` | 4 | PASS | 152.02 |
| `esp32c3-probe-default` | 4 | PASS | 151.65 |
| `esp32c6-probe-webui` | 4 | PASS | 151.39 |
| `esp32c3-probe-webui` | 4 | PASS | 143.34 |

## Independently observed local-terminal results

The user's terminal displayed 50/50 Local CI regression tests PASS,
`tools/validate_contracts.py` PASS, two repeated `--check` preflights PASS,
and the integrated full Docker runner exit code 0. These are user-supplied
terminal evidence; the first three are not recorded as individual test cases
in the uploaded JSON.

## PowerShell 7 → WSL2 boundary

PowerShell 7.6.7 / Core and running Ubuntu-22.04 WSL2 were observed.
The Windows launcher reached the Linux runner and identified the correct SHA
and Git tree, but its `--sudo-docker` image preflight ended with
`INTERACTIVE_AUTH_REQUIRED` and exit 1. Docker-dependent suites in **that
PowerShell invocation** were `NOT_RUN`. The previous successful WSL2 full
run is not invalidated by this distinct execution-context permission denial.

**Do not** modify sudoers, the Docker group, socket permissions or
authentication flows to manufacture a PowerShell PASS. The current launcher
honors a noninteractive, fail-closed authorization boundary.

## Canonical GitHub identity checked

- `main = 6393c84b5a46e619449e8938b9816535500d58f7` (unchanged).
- Integration branch `4a92b0bdccf9075cd64eaccec8f398b2649961f5`.
- Integration branch Actions runs: 0; check-runs: 0 (NOT_RUN/UNVERIFIED).
- Draft PR #13 remains open and unmerged.

## Boundaries / next owner

**Integration is not admitted as GitHub required-check PASS.**
Fresh-machine rebuild, native Linux validation, complete Windows launcher
Docker preflight, and required independent GitHub CI remain outstanding.
Stage 3 Gate A remains PENDING / UNPROVEN; Gate B remains ON HOLD.
No PR, merge, GitHub Actions execution, or new repository mutation
was performed by this evidence review.

Recommended coordination state:
`INTEGRATION CANDIDATE USER-VALIDATED / SUMMARY AUDIT PASS /
 PS7 DISPATCH PASS–AUTH BLOCKED / GITHUB REQUIRED CHECKS UNVERIFIED /
 MAIN INTEGRATION HOLD`.

**Publication scope:** This review note is published on an isolated coordination branch commit.
The exact integrated code/tree that was actually run and verified remains
`4a92b0bdccf9075cd64eaccec8f398b2649961f5` /
`00dc010774c08a8d65c6855915028a23eb951f75`.
A later coordination-only commit changes the full Git tree; it does not
retroactively acquire a Full CI PASS for its new commit identity.
