# GATEA-D3 Attempt-02 — uploaded archive independent review

Date: 2026-10-27 (Asia/Taipei)  
Sole Hot: `GATEA-D2`; active child: `GATEA-D3_DESIGN_VERIFICATION_OFF_REPO_FAKE_TRANSPORT_USER_AUTHORIZED_20261027`.  
**Disposition:** `ATTEMPT02_ARCHIVE_VERIFIED / ROOT_GUARD_FAILURE_UNCLASSIFIED`. No D3 model-verification acceptance, no automatic attempt-03.

## Independently verified from uploaded bytes

- `handoff-corrective-02.zip`: 59,382 bytes; SHA-256 `7eca88727d7259a41edf37d8d9a290e3ad87fcda764df3f7f3782d49c612d6d3`. 32 unique regular ZIP file entries, CRC PASS, no traversal/symlink paths.
- `manifest-corrective-02.json`: **31/31 payload lengths + SHA-256 exactly match**; 32nd member is self-excluded manifest.
- `handoff.zip` nested original ZIP SHA-256 `899d8899ada25c1774b2c4ad9b4c7db23a1df8fc5efc6c39fde4af3f2cb2973a`; nested CRC PASS. The original 17 immutable root entries listed in `preflight-02.json` all byte/hash match the corrective archive and after-execution producer readback; overlapping original ZIP entries byte-match its embedded original copies. These checks concern preserved **uploaded bytes**, not direct NTFS live file access.
- Corrective `run_d3_r2.py`: SHA-256 `86df68bc1ed490f6fbd10fb611bcf20090d9947b01815558d7cea0a381262fc8`; original `run_d3.py`: `8b4914d0c227231b0a8caa3ce7a924a3779e8551c9db9b557e1f186c753cfb43`. Both AST-parse successfully; independently reproduced **byte-identical** unified patch SHA-256 `231986d676bcb30ab5dfe0bcf7e95e59c3e998fb23f41f49fe459efb6e3db370`. No bundled code was executed.
- `process-02.json` supports **producer-reported** started=true, exit code 1, 167ms, watchdog not triggered; independently verified `stdout-02.txt` is exactly 0 bytes; `stderr-02.txt` is exactly the 34-byte `STOP: wrong stable evidence root\r\n` with matching recorded SHA-256. `outcome-02.json` says case_count=0, meta_control_count=0, both **NOT_RUN**, attempt-03 NOT_RUN.
- 31/31 checks in the separate ChatGPT archive/readback/source-*consistency* audit PASS. **This is NOT 31/31 test cases**, and source-level guard/root failure remains **unclassified**. Independent audit JSON is a separate conversation artifact, SHA-256 `a4f5675dc6fb8347a60e7bfb68348eb62de766a309812e2f4c44c8af5d16d20d` (not committed to Git).

## Verified root-guard trigger, unknown cause

`run_d3_r2.py` lines 231–247 enter `main()`, validate arguments and enabled assertions, then at **235–237** compute:

```python
root = Path(__file__).resolve().parent
if root != Path(r"C:\Users\user\AppData\Local\wfrac-gatea-d3-evidence-20261027-recovery-01"):
    raise SystemExit("STOP: wrong stable evidence root")
```

This raises **before** reading/verifying original `run_d3.py` in the Python process, installing its audit hook, creating `evidence/attempt-02`, or invoking `cases()/meta()`. PowerShell launcher `launcher-02.ps1` pins the same expected root literal as `$r`, checks root-probe and immutable files, and launches the corrective using `Join-Path $r 'run_d3_r2.py'`. This difference of *literal source path supplied* vs *Python-resolved path* does not prove the two paths are the same resolved NTFS object. Neither `process-02.json` nor stdout/stderr records the actual Python-resolved `root`; producer explicitly writes `UNAVAILABLE`.

**Classification:** The newly added check is the *immediate error emitter*. Exact underlying reason is **UNKNOWN**, potentially physical path alias, sandbox mapping or true mismatch. No supported basis yet to call it a *confirmed* repairable harness defect or a valid preservation/root-integrity rejection.

## Scope/quality of derivative, NOT executed

Static source contains plausible targeted changes to `case()` oracle, `proof_negative()/meta()`, actual zero/negative constructor budget, distinct symbolic pending/failed `SO_ERROR` states and a fake-ready path. **None of these corrected branches were run** in attempt-02. `static-source-review-02.json` reports source-only scope PASS, but that is not evidence of model verification acceptance. The old 52/52 case + 4/4 fixed boolean meta results are **historical run-01 only**.

`source-guard-table.json` remains the original transcribed research, with uncorrected G6 path uncertainty; no new pinned ESP-IDF native proof. Real `D_physical`, `D_cleanup`, SDK TLS/DNS/callback quiescence and secure firmware behavior remain unproven.

## Decision, no new executable authorization

- **Attempts used: 2 of 3** (run-01 model-only PASS record; run-02 **FAIL_BEFORE_CASE_EXECUTION**, 0 cases). Last attempt-03 is **NOT automatically authorized** under existing condition requiring specific evidence that run-02 failure is a bounded repairable harness defect; root cause remains unknown.
- Next minimum candidate: **read-only, no-harness, no-write, same-approved-execution-context path-identity diagnostic** of Python `__file__`, `Path(__file__).resolve().parent`, expected root `.resolve()`, current process CWD, Windows path identity/NTFS/reparse and root-probe in the same stable root, with **redacted safe outputs**. Do not infer canonical equivalence from case-folding alone or disable the root guard. Scope/permission must be confirmed separately before any execution. Do not run attempt-03, create root number three, overwrite the original, modify Git trust/ACL, or start SDK/CI/network/hardware.
- Keep Stage 2R **CLOSED / TRACE_LIMITED**; D3 **PARTIAL/BLOCKED**; original HTTPS map mismatch UNRESOLVED; Gate A **PENDING/UNPROVEN**, Gate B **HOLD**, Stage 4 **COLD**, library readiness **PRESERVED**.
