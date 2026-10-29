# GATEA-D4 attempt01 — exact single native C6 build authorization, awaiting user WSL execution (2026-10-29)

**Status:** `D4_ATTEMPT01_EXPLICITLY_AUTHORIZED__HOST_NOT_RUN`. Sole Hot `GATEA-D2`, unchanged child `GATEA-D4_PINNED_SDK_CUSTOM_TRANSPORT_COMPILE_LINK_ONLY_USER_AUTHORIZED_20261027`. The user replied 「授權」 immediately to the explicit scoped question whether to approve a **single** isolated D4 attempt01 container create/start plus native ESP32-C6 Custom Transport compile/link; the approval excludes ELF execution, internet/network, hardware, CI, production-source changes, D2 volume mounts/changes, SDK version changes, volume reset, automatic retries or extra Stage.

## Frozen host preparation and build identities

- Latest project main observed before admission: `f64e440670aaa382bd3aaa7e94f3a63295e81851`; Shared Playbook baseline main resolved to `29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70d`.
- Prior user WSL prestart with exact script hash `5464f49d6838326d08dcfbb88c19bcb3222dc89e546841436f13e3de39b6777e` reported pinned source 17+7, Docker image/volumes, empty D4 volume, absent container name and read-only completion exit 0. It is a time-scoped host observation, not a future guarantee; pre-create repeats the same guarded prestart.
- Exact D4 LocalCache root is `/mnt/c/Users/user/AppData/Local/Packages/OpenAI.Codex_2p2nqsd0c76g0/LocalCache/Local/wfrac-gatea-d4-preparation-20261028-01`; only RO bind mount to `/src`. The validated `d4-v2/build-c6-isolated.sh` has SHA-256 `4fcd675ffc75184900320cf6584875ed0c6cd2230083876011aa2a9bf4b5884e`.
- Docker image `sha256:0d8c9773d48a327233f9c1d7c654ff0bcf133ae24503ea2e97a57cfe02b8cb67` linux/amd64, Docker Server 29.1.3. Inside guest, build script pins ESP-IDF commit `7101770dc6db2667b3c477cc31365dd1acd6db4e`, lwIP commit `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`, SDK Git blobs and the C6 compiler SHA `a0dfebec55dd9c6819cd554262c8d1dee784a2a0724af134ebdbc9190aa763aa`.
- Existing D4-only RW volume `wfrac-gatea-d4-c6-20261028-01` to `/out`, observed ext4. Protected D2 `wfrac-gatea-d2-fs-20261021-01` is **never mounted**. Intended container name `wfrac-gatea-d4-c6-v2-attempt01`; in-container GNU timeout 1200 sec, external attached-client timeout 1230 sec. No network, IPC, devices, extra bind mounts or capabilities; readonly rootfs, tmpfs `/tmp`, no-new-privileges, pids-limit 256. ELF is compiled but **never executed**.

## Audited host command artifact and modes

An isolated ChatGPT working container created `d4-native-attempt01-controlled-20261029.sh`, **9,709 bytes**, SHA-256 **`054259ea26a5bf994b6c6a2b9ac40813e067c8a5957ec4b261ea7f9823bb2ed2`**. Static report `d4-native-attempt01-static-review-20261029.json` SHA-256 **`b09a44f02dfcacc1c1887e3a6d1497354733e4343c33ce22f99b29107c2b7531`**. Bash syntax PASS; 14/14 bounded static guards PASS; 9/9 mock Docker-inspect accepted/rejected cases PASS, including D2 mount and network/privileged/wrong-image rejection. **No real Docker daemon or SDK was called in ChatGPT runtime; these are not real native test results.**

The script has four independently invoked human WSL modes:

1. `create`: re-check sealed LocalCache 17+7 hashes and current daemon/image/D2/D4 volume metadata, run the previous SHA-pinned readonly prestart, insist on empty D4 volume and absent container. Then perform **one** `docker create` only, using existing D4 named volume with `volume-nocopy`. No compiler runs at this stage.
2. `inspect`: independently JSON-inspect container image, entrypoint, exact 1200 sec command, environment approval, /src RO and D4-only /out RW mounts, rootfs/network/capability/privilege/IPC/tmpfs limits. Require state `created` and D4 volume still empty. Any mismatch STOP and **do not start**; preserve container.
3. `start`: independently repeat the exact inspect check, verify state `created` and D4 volume still empty, then execute exactly one bounded `docker start --attach`. This is the sole **attempt01 consumption boundary**; on any failure/timeout capture exit and **do not retry or remove container/volume**.
4. `postcheck`: after start, separately read container state/exit/OOM, expected output filenames, selected evidence hashes and guest exit-code; preserve all output. This is **not** proof of native compilation success or acceptance without actual source/ELF/map/symbol inspection.

**Human operator safeguards:** SHA-verify and review the delivered candidate before execution. No automatic chaining across modes; manually proceed only after explicit PASS at each boundary. If a step fails, do not repeat `create` or `start`, change permissions, delete/recreate volumes/containers, switch roots, or perform additional test attempts. An authenticated sudo prompt stays entirely on the user's WSL terminal. In particular, a newly created but not-started container is retained on inspect failure. After any attempted start, run read-only `postcheck` even when the start exit is nonzero; report exact outputs to ChatGPT for independent evidence evaluation.

## Current evidence and prohibitions

**As of this GitHub authorization checkpoint, no post-authorization real WSL Docker create/start or native C6 compile/link result was supplied.** Stage-local D4 attempt remains `NOT_RUN / 0 of 2`. The user’s authorization allows only attempt01 under this exact gated contract; attempt02 is **not** authorized. No Gate A promotion follows automatically: Gate A `PENDING/UNPROVEN`, Gate B `HOLD`, Stage4 `COLD`, D3 CLOSED 3/3 and library readiness `PRESERVED`. GitHub coordination mutation does not equate to host execution.
