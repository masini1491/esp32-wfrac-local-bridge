# GATEA-D2 A3 — Codex WSL bwrap pre-exec launcher diagnostic (2026-10-30)

**Disposition: `A3_LAUNCHER_SOURCE_TRIAGE_ONLY__NO_RUNTIME`.** This is a bounded external-source-informed diagnostic, **not a host observation**. There is no new WSL/Codex command output, no sandbox proof, no launcher repair or installation, and the single A3 attempt remains **0/1**. The original Corrective02 raw records are still unavailable.

## Current project evidence

- Sole Hot GATEA-D2, current conditional A3 child `GATEA-D2-NAMESPACE-A3-DURABLE-RECEIPT-ONE_START_20261030`; target baseline `984374e9b6aa23a6e0f934dc9e8a02e71e98df26`; declared shared Playbook main observed `ecf7198cd5f21878d10d04e844a99b70f7925438` for this design note.
- Previous bounded Codex receipt-export launch was reported to fail **before any shell command** with missing bwrap. Its exact launcher log / exception code was not provided. Earlier passive WSL inventory separately observed `bwrap` absent from PATH, but rootless `unshare` had succeeded during an older Codex execution session. These facts do not isolate a single root cause.

## Source-backed distinctions (external, not local verification)

- Canonical source `openai/codex@788b9d7bf6e37f735647b7f84b556c2a807df4d8` `codex-rs/linux-sandbox/README.md` and `src/launcher.rs` describe a preferred system `bwrap` and fallback `codex-resources/bwrap` bundled helper. `src/bundled_bwrap.rs` discovers an executable bundled resource and verifies its SHA256 before exec. The warning text in `codex-rs/sandboxing/src/bwrap.rs` explicitly says the bundled helper will be used if system bwrap is not found. These mechanics are **source version-specific**: installed Codex Desktop/WSL engine version and actual runtime log have not yet been identified.
- Public Windows/WSL reports `openai/codex#54478` (opened 2026-10-16), `#50025` (opened 2026-10-01), `#29333` (opened 2026-06-21), include `bundled_executable_relocation_failed`, source `codex-resources\\bwrap` missing with `ENOENT`, or shell failing before first command. At least one report says installing system bwrap did not fix its separate Desktop WSL startup fault. Other reports show relocation warnings coexisting with unrelated failures. **Similar symptoms are not identity/root-cause proof**.
- Distinguish: `bwrap` not on PATH; bundled source missing from Windows MSIX resources; relocation destination missing/permission failure; old/stale launcher helper; or another early failure. Do not promote any hypothesis until local version/source/destination/log evidence aligns.

## Authorized next diagnostic only

Use ordinary user-run Windows PowerShell **read-only** metadata inspection, *outside Codex Agent*, of (a) `Get-AppxPackage -Name OpenAI.Codex` installed version/InstallLocation; (b) existence only of `app/resources/codex-resources/bwrap` at the selected package path and `%USERPROFILE%/.codex/bin/wsl/codex-resources/bwrap`; and (c) exact October 30 Codex Desktop log location and bounded counts of `bundled_executable_relocation_failed`, `stat_source`, `remove_destination`, `ENOENT`, `EPERM`, `bwrap`, and nearby actual fatal process-launch errors **without emitting secrets/paths/raw logs**. If installed package isn't resolvable, report UNKNOWN; don't guess.
- Must NOT call `bwrap`, `unshare`, `codex exec`, `wsl.exe --exec` or run any executable test in this diagnostic. No `apt`, `winget`, admin, mode/permission changes, symlink/copy/repair, cache deletion, WSL settings, git write, network, alternate launcher or A3 namespace test.
- The user can paste a redacted, bounded diagnosis (package version, file-presence booleans, log source and exact error classifications); ChatGPT then decides if further explicitly authorized **normal product repair** is warranted or if a known-good approved launcher is actually restored. A3 active execution remains conditionally blocked until launch source, security boundary and durable byte-level receipt sink independently pass their gates.

**Status remains:** `USER_AUTHORIZED__STAGED_PREEXEC_BLOCKED_ON_LAUNCHER_AND_DURABLE_RECEIPT`, A3 0/1; Corrective02 `NAMESPACE_A_REPORTED_PASS__RAW_RECEIPT_REVIEW_PENDING`; Guard B/C NOT_RUN; target ELF NOT_RUN; project Gate A PENDING/UNPROVEN, Gate B HOLD, Stage 4 COLD, library readiness PRESERVED.

## External primary source pointers

- https://github.com/openai/codex/blob/main/codex-rs/linux-sandbox/README.md
- https://github.com/openai/codex/blob/main/codex-rs/linux-sandbox/src/launcher.rs
- https://github.com/openai/codex/blob/main/codex-rs/linux-sandbox/src/bundled_bwrap.rs
- https://github.com/openai/codex/issues/54478
- https://github.com/openai/codex/issues/50025
- https://github.com/openai/codex/issues/29333
