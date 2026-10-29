# GATEA-D2 — V3 SDK Transport Lifecycle test-only correction (2026-10-29)

**Current outcome:** `D2_SDK_LIFECYCLE_V3_TESTONLY_USER_AUTHORIZED__HOST_PREFLIGHT_NOT_RUN`. This is a new bounded test-only continuation explicitly approved by the user in the current chat after the V2 one-shot `exit 126`; it does **not** resurrect or retest the V2 container. Same sole Hot `GATEA-D2`. No production adapter/source edit, no network/HTTP/HTTPS/TLS/hardware action, no Stage 4, no original D2/D4 volume access.

## Provenance and frozen identifiers

- Project main at admission: `e2c48584680bd4c692ee0e4a721c48fe030686a8`.
- Project Playbook baseline `masini1491/ai-development-playbook@29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70d` (current declared `main` resolved exact).
- Existing V2 sealed ZIP `d2_lifecycle_sdk_source_testonly_v2_20261029.zip`, SHA-256 `a0289998412a56a6913a02b4e501f01a6abc062f1580d84f47d5850655ec3b6e` was independently rechecked before deriving V3. V2 retains its original namespace/container, one start consumed with `exit 126`; true SDK lifecycle assertions V2 `NOT_RUN 0/4`.
- New off-repository V3 ZIP `d2_lifecycle_sdk_source_testonly_v3_20261029.zip`, **9,518 bytes**, SHA-256 **`0bd3c3b7f4be98ed4390ee374dc87c5c6c84a2f3de93803fcca4fd9bda4f0877`**, 10 ZIP members, internally manifest-pinned 9 files, ZIP CRC PASS. Unique extraction folder `d2_lifecycle_probe_v3_20261029`; new Docker name `wfrac-gatea-d2-lifecycle-testonly-v3-20261029-01`. The V3 bundle contains test-only stubs/harness/shells, **not the upstream SDK source**.
- Existing pinned SDK Docker image remains `sha256:0d8c9773d48a327233f9c1d7c654ff0bcf133ae24503ea2e97a57cfe02b8cb67`, Server `29.1.3`, SDK `7101770dc6db2667b3c477cc31365dd1acd6db4e`, exact previous four SDK blob digests unchanged. No automatic image pull.

## Exact V3 changes vs V2

1. Distinct new host bundle folder and container name; V2 files/container and previously protected named Docker volumes are never referenced as mounts or mutated.
2. **One bounded Docker `create` only** now explicitly uses `--tmpfs /tmp:rw,nosuid,nodev,exec,size=96m` while retaining one `/probe` read-only bind, `--network none`, read-only rootfs, caps drop all, no-new-privs, fixed PIDs/memory/CPU and `/usr/bin/timeout 180s`. Docker's documented `exec` option permits executable binaries in tmpfs. This is a correction candidate; it does **not** prove the V2 effective mount flags were `noexec`.
3. Independent `host.sh inspect` validates the literal `.HostConfig.Tmpfs` value is the exact new configuration as well as the existing network/mount/permission/name/created-state guards.
4. `guest_run.sh` verifies SDK revision and four blobs, compiles the unchanged real vendor `transport.c` plus test-only C harness into private container `/tmp`, then **before execution** checks actual `/proc/self/mountinfo`: there is one `/tmp` tmpfs, it is rw, and effective `noexec` is absent. Then it rejects missing/nonregular/symlink executable, absent execute bits and `os.access(X_OK)` failure. Stops without changing permissions/mounts. Only if both checks PASS may it invoke the binary and report SDK dispatcher results.
5. Modes are separate operator calls `preflight`, `create`, `inspect`, `start`, `postcheck`. No automatic chaining, no retry/restart/cleanup/delete/remount; V3 `start` is one-time and does not authorize a second start if execution fails.

## Current validation tier

In ChatGPT's isolated container (not the user's WSL Docker), the new V3 ZIP was created from the SHA-pinned V2 kit and validated with `bash -n` for both scripts, `sha256sum -c` for 9 files, ZIP CRC/member inventory and **23/23 bounded static plus synthetic checks PASS**. Synthetic positive test verified the guest mount/mode gate accepts a rw tmpfs without noexec and an executable regular file. Negative cases reject noexec, readonly, wrong filesystem, missing /tmp mount, missing executable bits and symlink. The host command surface has a single create and a single start mode, exact image/network/no-volume/permissions guards, plus the new literal tmpfs inspect guard. The separate `d2_lifecycle_v3_isolated_validation_20261029.json` report SHA-256 is `4070d2f64fb0f17d38d940c480c169e16133c3beb0c14987a057082ad378c6c4`. **These checks simulate mountinfo and do not imply actual Docker/SDK execution or prove vendor runtime assertions.**

## Authorized next host actions / STOP boundaries

The user has authorized this *new* V3 test-only correction and one separately gated Docker container attempt. At the point of this evidence commit, the V3 folder has **not** been extracted or probed in the user's WSL. First operator checkpoint is: external ZIP SHA match, destination folder absent before extraction, internal 9/9 manifest, `bash -n` both files, and `bash host.sh preflight` with exit 0. Only after its output has been reviewed can the operator perform one `create`, separate `inspect`, then one `start`, followed by read-only `postcheck`, with STOP on every unexpected result. Preserve any created container and all logs even after failure. No additional start or remedial permission/volume change is approved.

**Higher gates unchanged:** `GATEA-D2` Sole Hot, D3 model CLOSED 3/3, D4 native build CLOSED 1/2, Gate A `PENDING/UNPROVEN`, Gate B `HOLD`, Stage 4 `COLD`, Library readiness `PRESERVED`. This test only assesses the pinned `esp_transport` dispatcher lifecycle under fake callbacks; it cannot establish HTTP client cleanup, quiescence, TLS, deadline, or full Gate A readiness.
