# CI-PORTABLE-001 — Rebuildable local CI: Codex implementation contract

Status: USER-AUTHORIZED / CODEX IMPLEMENTATION ADMITTED; implementation NOT_STARTED.
Repository: masini1491/esp32-wfrac-local-bridge; canonical Hot pointer: TASKS.md → CI-OPT-001 / CI-PORTABLE-001.
Producer-observed base: main@c7c7349c781a96ca615be105a495d73375658160; shared Playbook main@1cff3ae6bc8b4d7245e0e2733b1e8f64c5d75614. These are continuity references, NOT frozen current authority. Receiving Codex must fresh-fetch and reread.
Scope is Stage 3 CI-budget prerequisite. This staging document is not an authority expansion.

## Goal
A new Windows 11/PowerShell 7/WSL2 Ubuntu 22.04 machine or native Linux x86_64 machine can fetch this repository, let Codex follow AGENTS.md and TASKS.md, inspect prerequisites, ask permission before privileged installation/login, rebuild pinned local validation, and run safe offline profile-specific tests with machine-independent source/version/time/outcome evidence. No additional billed GitHub-hosted Actions minutes; personal electricity/storage/data usage still exist. Rebuild is not an automatic replacement of formally required GitHub CI.

## Minimal Codex implementation

Proposed bounded artifacts, names may vary within admitted scope when justified:
- tools/bootstrap-local-ci.ps1: Windows PS7 nondestructive preflight and WSL2 handoff; no auto feature enable or silent elevation.
- tools/bootstrap-local-ci.sh: Linux/WSL2 prerequisite inspection, Docker+systemd, architecture, image availability and SDK integrity; safe idempotent --check default; any package install or pull is a separate human-approved action.
- tools/local-ci.sh: explicit targeted/full or exact-suite offline runners and timed PASS/FAIL/NOT_RUN summaries; do not silently omit suites on unknown scope; avoid assuming unmerged PR #13 ci_scope.py is on main.
- tools/local-ci.config (or equivalent): checked-in no-secret manifest for espressif/idf:v6.0.2 and exact SDK/LwIP SHAs; treat digest as platform-specific and verify observed image identity, not floating trust.
- docs/LOCAL_CI.md: new-machine minimal setup, PS7 to WSL2 vs native Linux, Git/gh login boundary, safe cache use, exact run instructions, failure recovery, evidence semantics and no-silent-install controls.
- tests/local_ci/** minimal deterministic regression fixtures, negative tests and schema/accounting checks.
- Optional small AGENTS.md pointer solely for discoverability; DO NOT change ChatGPT+Codex mode, write allowlists, runtime/network/security governance.

## Safety and test fidelity
- Always verify identity, expected branch/HEAD and clean tree using current safe-remote-sync discipline before mutation. Original Windows checkout had seven unrelated uncommitted modified files; separate Linux clone was detached at PR #13 candidate. Do not reset --hard, git clean, auto-stash, force push, switch over a dirty tree, overwrite folders, or treat alternate clone as permission bypass. STOP if no eligible clean workspace; ask user for a permitted workspace.
- Verify image, IDF version ESP-IDF v6.0.2, IDF commit 7101770dc6db2667b3c477cc31365dd1acd6db4e, lwIP commit fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e. Mismatch STOP, not automatic SDK upgrade.
- Match current workflow commands and gates: .github/workflows/validate-contracts.yml full Python/C++ Host suite including D1 pinned-parser acquisition; .github/workflows/esp-idf-compile.yml A6b Linux real callback runtime and config/symbol check; A7b DNS zero-egress interception proof and 7-case SDK handler run; firmware matrix on esp32c3 and esp32c6 with idf_compile_smoke(default), wfrac_bootstrap_probe(default), wfrac_bootstrap_probe(webui): SIX total cells; every cell additionally A5 isolated SDK callback-fence compile/config comparison/link-symbol verification. Preserve negative assertions (WebUI default-off; fixture password only), compiler and toolchain checks.
- D1 --fetch-pinned-parser originally requires acquisition; if not prefetched, request explicit user approval for a separate bounded pin-verified download or report Host D1 NOT_RUN/STOP; no silent network from validation. Do not hardcode secrets or copy user credential directories.
- Validation containers: --network none, source read-only, disposable and safe build output; no real network/hardware, device Wi-Fi/HTTP/TLS, privileged Docker container, Docker socket mount, SDK patch or emulator. Do not globally relax host safe.directory or enroll Docker user into root-equivalent docker group without approval.
- Bootstrap/check is read-only by default; no apt upgrade/install, OS feature enable, Docker Hub pull, GitHub browser login, credential mutation or arbitrary checkout changes without explicit human permission. GitHub API/CLI only for separately authorized repo fetch, not tests.
- Logs generated outside tracked tree (or otherwise gitignored), sanitized and size-bounded. Results: repository and exact candidate SHA/tree when material, pin/image digest/arch, profile+suite+target/variant, actual executed command identity, PASS/FAIL/NOT_RUN, wall time (real), relevant negative/timeout and missing-validation statuses. Failed/absent tool, unsupported target, wrong SHA/image, skipped suite cannot become green. Full-profile completeness means all 6 firmware + all 6 A5 actually run; otherwise label partial/not-run.
- Safe cache strategy optional after proving reproducible cold build, invalidation on SHA, image/config/target changes, no cache as substitute for SDK runtime proof.
- Single common source of validation truth is existing GitHub Workflow; script wrapper may point to/extract stable commands or use shared helpers without modifying production CI YAML in this Stage. If true parity would require Workflow changes, STOP and request separate scope authorization.

## Acceptance checks
1. --check preflight on prepared environment is non-destructive; missing WSL2/Docker/Git/SDK gives clear human instructions and STOP.
2. Two runs on provisioned host are idempotent; no unnecessary downloads, reauth or privilege escalation.
3. PS7/WSL2 dispatch verified if supported; native Linux x86_64 path documented. Unsupported OS path marked NOT_RUN, not PASS.
4. Negative tests: dirty tree preservation, head identity drift, wrong target, invalid/unknown change scope, missing image/SDK SHA, Docker failure, wrong runner exit code, missing suite, clean separation PASS/FAIL/NOT_RUN, offline guard and output redaction.
5. Offline selector (where present/verified) and Host suite; A6b 3/3, A7b 7/7, and both C3/C6 smoke builds reproducible on intended source SHA, with timing. Six-cell full plus per-cell A5 must retain execution plan and truthful NOT_RUN if not yet executed; never claim full gate prematurely.
6. git diff --check, test suite, candidate exact SHA and changed file allowlist; independent ChatGPT review before acceptance. No remote PR/merge without separate user authorization.

## Observed inputs, not new test results
User-provided local session 2026-10-15: Ubuntu 22.04 WSL2 + systemd, Docker Engine 29.1.3, espressif/idf:v6.0.2 image, pinned ESP-IDF/lwIP SHA verified. PR #13@2ceed6ec803a849229f17c3932440e56a8076e89 local selector 20/20, A6b 3/3, A7b 7/7, C3/C6 idf_compile_smoke compile/link all PASS; C3 real 1m8.879s. User logs are conversation-provided local evidence, not fresh target-main candidate tests or formal PR-required CI. Full Host, A5 matrix and GitHub CI restoration remain unproven.

## STOP and release guard
CI-OPT-001 PR #13 remains OPEN/DRAFT/UNMERGED and GitHub Actions quota/billing BLOCKED until independently recovered. No auto workflow retry, Actions spending, self-hosted runner registration, PR #13 modification/merge, production Gate B, device runtime, new deps/security relaxations, GitHub Workflow edits or shared Playbook mutation. Implementation in authorized clean isolated branch; local candidate commits only for now; ask before push or PR that could start hosted Actions. Codex reports candidate SHA/paths/results and STOP blockers, then ChatGPT reviews independently; integration always separate approval.
