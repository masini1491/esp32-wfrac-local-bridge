# Rebuildable local validation (CI-PORTABLE-001)

Local evidence does **not** replace formally required GitHub checks, independent
ChatGPT acceptance or separate integration approval. No self-hosted runner is
registered. These commands never push, open a PR or trigger Actions. PR #13's
unmerged selector is not a dependency. Production/network/hardware remain outside
scope; A7b runs only its existing in-memory, zero-egress SDK handler tests.

## New-machine prerequisites / permission boundary

- Windows: PowerShell 7/Core (`pwsh`), WSL2 Ubuntu 22.04; or native Linux x86_64.
- Git, Python 3 + PyYAML, Docker Engine with explicitly authorized socket access.
  Inspect actual executable/version/smoke results; presence alone is not READY.
- Existing `espressif/idf:v6.0.2` Linux/amd64 image and a separately reviewed
  platform-specific RepoDigest. The tag alone is not trusted. No automatic pull.
- The manifest requires ESP-IDF `7101770dc6db2667b3c477cc31365dd1acd6db4e`
  and lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e` plus D1's two original
  parser blob identities. The image must already contain the required compilers,
  SDK Python environment and parser; missing prerequisites stop, not download.

OS/WSL/Docker/PyYAML installation, image pull, service start, Git/gh login or other
privileged setup needs a separate human-approved operation. Never submit passwords
or tokens to chat/scripts. No sudo fallback: `--sudo-docker` is an explicit,
separately approved choice using `sudo -n docker` only. If it needs a password,
STOP; authenticate privately in your own terminal and execute there. A sudo ticket
in another terminal need not authorize this process. Never change docker group,
socket permissions, global Git config or sudo policy for convenience.

All noninteractive child processes use closed stdin. Interactive sudo/su/login
commands are rejected before launch. A password-required response to an explicitly
approved `sudo -n` probe is `INTERACTIVE_AUTH_REQUIRED`, not SOURCE failure or
permission to elevate further. Use the reported minimal command only in your own
terminal; no passwords in prompts, files, environment or logs. After human action,
preflight runs again (no cached authorization claim). Continue deterministic offline
tests while Docker-dependent validation remains NOT_RUN; never enable passwordless
sudo or change sudoers, groups or socket permissions from this tooling.

Before work, follow AGENTS identity/safe-sync rules. Scripts do not fetch or change
refs. Preserve dirty trees; no reset/clean/stash/auto-clone. Use the intended clean
checkout and explicit branch/SHA. A Linux checkout with LF scripts is preferred;
do not copy secrets/credential directories or overwrite another checkout. Windows
checkouts must preserve LF for `.sh`; do not rewrite user files to fix CRLF.

## Check / inspect (no builds)

Run from the repository root. `SHA`, `BRANCH`, `DIGEST` below are deliberate
placeholders for the exact reviewed candidate and platform digest, not fixed epochs
or floating defaults. Do not execute placeholder values.

```sh
bash tools/bootstrap-local-ci.sh --expected-branch "$BRANCH" --expected-sha "$SHA" \
  --image-digest "$DIGEST"
bash tools/local-ci.sh --plan --profile full --expected-branch "$BRANCH" --expected-sha "$SHA"
```

`--check` is the default. It inspects existing Docker metadata and starts only a
disposable **offline read-only inspection container**, not a build. It never pulls,
installs or repairs. Run it twice on a prepared host to check idempotence. Image
inspection is initially untrusted; actual execution is by frozen content ID, with
digest/architecture and original SDK source pins checked. A missing image/digest,
Docker permission failure or SDK mismatch is a blocker, not source failure/PASS.

Windows handoff (no silent PS5.1 fallback):

```powershell
pwsh -NoProfile -File tools/bootstrap-local-ci.ps1 -Distribution Ubuntu-22.04 `
  -LinuxRepoPath /absolute/linux/repository -ExpectedBranch $Branch `
  -ExpectedSha $Sha -ImageDigest $Digest
```

Omit `-LinuxRepoPath` only when the current Windows checkout can be safely mapped
with `wslpath`; a different Linux checkout must independently satisfy identity,
branch/clean/SHA requirements. This does not authorize changing either checkout.

## Explicit offline execution

```sh
bash tools/local-ci.sh --run --profile targeted --expected-branch "$BRANCH" \
  --expected-sha "$SHA" --image-digest "$DIGEST"
bash tools/local-ci.sh --run --profile full --expected-branch "$BRANCH" \
  --expected-sha "$SHA" --image-digest "$DIGEST"
```

- `targeted`: full Host, A6b, A7b, C3/C6 smoke **including their A5 gates**.
- `full`: Host, A6b, A7b and all SIX C3/C6 firmware/A5 cells (smoke/default probe/
  WebUI for both targets). No cell can be omitted and called full PASS.
- `--suite host|a6b|a7b|esp32c3-smoke-default|esp32c6-smoke-default|...` selects one
  exact plan ID. Inspect `--plan --profile full` for all IDs. Unknown scope fails;
  no PR API/automatic selector or silent full-profile fallback is implemented.

The source of commands/conditions/matrix/check wiring is the two current workflow
files. Canonical Git blob hashes in the manifest fail on **any** drift. Independent
review must reconcile changes before repinning; do not update hashes just to pass.
Only existing checkout actions are replaced by verified local source. D1's explicit
`--fetch-pinned-parser` becomes its existing `--sdk "$IDF_PATH"` mode, which validates
the identical original blobs. No parser is imitated and no validation-network
acquisition is attempted. If workflow parity needs YAML edits, STOP for new scope.

## Isolation, outputs and evidence

Every run uses `--network=none --pull=never --read-only`, nonroot host UID/GID,
all capabilities dropped and no-new-privileges. Source is a read-only bind; a
disposable output directory outside the repository holds builds/logs/cache. No
Docker socket, host home/credentials, devices or privileged container is mounted.
The writable output facade symlinks original sources so unchanged relative compiler
commands can write binaries without modifying source. Python caches go to output.
SDK/project Git trust is scoped to three verified container paths in process-local
environment only; host/global trust is never changed. No cache reuse is assumed.

The runner prints `OUTPUT:`. Preserve its `result.json`, per-suite provenance,
step command hashes/status/wall seconds and bounded sanitized logs. It records exact
SHA/tree and image identity. Private keys/recognized credential lines are redacted;
this is defense-in-depth, **not** permission to use secret-bearing inputs. Long
lines are discarded and retained logs bounded; no raw log is written first.
A recognized sensitive marker (including a JSON/YAML key with a multiline value),
private-key header or oversized/unclassifiable line suppresses the rest of that
log stream. This deliberately sacrifices later diagnostics instead of guessing
where a credential ends; it is not a universal secret detector.

After repository/workflow identity checks, an external output directory and initial
`result.json` report state are established before Docker image/auth preflight.
Preflight failure is persisted as overall FAIL, suites/full profile NOT_RUN, with
`error_code=INTERACTIVE_AUTH_REQUIRED` when noninteractive authorization is missing.
No password prompt or automatic retry follows that gate.
`image_preflight` describes only Docker image identity/access. It is not an SDK
integrity claim: `sdk_integrity` records each container's independently checked
SDK/lwIP/tool provenance. Missing/invalid SDK evidence is FAIL for that evidence
gate; containers not attempted remain NOT_RUN. SDK PASS alone is not suite PASS.

Nonzero/missing step, image drift, incomplete suite, timeout or dirty/source drift
cannot become PASS. Unselected/remaining jobs are NOT_RUN. A timeout attempts removal
of only that generated container; failed cleanup remains a blocker. Output is retained
for inspection; no broad deletion is performed. `full_profile=NOT_RUN` on targeted
success is intentional. Inspect per-cell A5 step execution, not just parent status.
On timeout, the attempted suite is FAIL and remaining suites are NOT_RUN; a requested
full profile is FAIL. The original timeout remains the primary `error`/`error_code`,
and the separate `cleanup` entry records success or failure without masking it.
Cleanup failure never triggers another privilege/authentication attempt or PASS.
A returned container with missing, malformed or incomplete `steps.json` is an
attempted suite FAIL (not NOT_RUN), even if SDK provenance passes and exit code is
zero. Unattempted suites remain NOT_RUN.

Job budgets are derived from workflow `timeout-minutes`, capped by an explicit
local policy where applicable (Host currently 1800 seconds; workflow has no
explicit Host timeout). SDK jobs retain 900/1200-second workflow budgets. The
outer container budget covers startup/export/integrity and steps; inside, all
steps share one monotonic deadline and receive only its remaining time, never a
new full budget each. `job_budgets` records the workflow/local/effective limits.
Process termination, log draining and separately reported cleanup may outlast a
budget; this is not a hard total caller-return or resource-quiescence guarantee.

GitHub required-check visibility/placement stays UNVERIFIED/unchanged. SDK handler
PASS does not prove network DNS, callback-token reclamation, bounded cleanup,
WF-RAC control, physical actuation or hardware readiness. Stage 3 remains pending
independent acceptance; Gate B is ON HOLD.
