# GATEA-D4 — Dedicated D4 output volume creation: scoped user admission (2026-10-29)

**Current status:** `D4_DEDICATED_VOLUME_CREATE_USER_AUTHORIZED__PENDING_HUMAN_WSL_EXECUTION`. Sole Hot `GATEA-D2`, current child `GATEA-D4_PINNED_SDK_CUSTOM_TRANSPORT_COMPILE_LINK_ONLY_USER_AUTHORIZED_20261027`. No new Stage.

## Explicit current authorization and limits

The user replied 「授權」 immediately after the scoped proposal to **create only** the fresh dedicated Docker volume `wfrac-gatea-d4-c6-20261028-01` under their existing WSL Docker daemon, after verification of absent name, daemon identity, actual Linux storage filesystem and separation from retained `wfrac-gatea-d2-fs-20261021-01`. This is permission to perform at most **one** manual human-terminal Docker **volume create** and exact scoped pre/post read-only checks; not permission to run Docker containers, mount either volume, copy/export data, execute the compiler, use CI, real network/hardware, change SDK/source, change host ACL/group/sudoers/socket or modify D2. Password (if requested by sudo) must be typed solely in the user's own terminal, never handed to ChatGPT/Codex.

## Audited human-terminal execution candidate (not yet run on user host)

ChatGPT prepared an off-repository, standalone shell candidate `d4-dedicated-volume-create-once-20261029.sh`, **4,578 bytes**, SHA-256 **`9b4951c49a6377eb4e1cbaef249da9a0ddc0011265eb1290745faeaca0b22e25`**, syntax-checked with `bash -n` in ChatGPT's isolated working container. This is static validation only, with no Docker daemon access or actual WSL invocation by ChatGPT.

The script is deliberately limited to a single `sudo docker volume create --driver local wfrac-gatea-d4-c6-20261028-01`, and all other Docker calls are `context show`, `context inspect`, `version`, `info`, `volume ls` or `volume inspect`. Preflight refuses wrong UID, Docker selector overrides, non-default/non-Unix socket endpoint, missing/symlink socket, server version other than previously verified `29.1.3`, inaccessible Docker root/volumes parent, missing/incorrect protected D2 metadata, non-ext4 backing filesystem at the visible Docker volumes parent, a registered D4 name, or a pre-existing D4 on-host directory. The script requires D2 driver `local` with its exact expected daemon-root mountpoint, then creates the one new D4 volume only once.

After create, it re-inspects D4 exact `Name`, `Driver=local`, expected daemon-root `Mountpoint`, `findmnt -T` ext4, and a distinct `stat` device:inode from the D2 directory; confirms D2 `Name|Driver|Mountpoint` remained unchanged. Success marker `D4_VOLUME_CREATED_AND_INSPECTED__NO_CONTAINER_OR_BUILD` is **conditional on actual human-terminal execution**. This does not prove protected D2 contents byte-for-byte unchanged; the script performs no D2 writes or mounts, and Docker metadata checks are an isolation prerequisite, not full volume forensic validation.

If any guard or readback fails, STOP immediately: do not repeat `volume create`, delete/recreate/rename volumes, change daemon/ACL/trust, switch context or Docker root, or attempt container/build operations. If failure happens **after** create, preserve the new volume for independent review; no cleanup or retry is authorized. Because Docker volume creation may reuse an existing name, the script checks absence in both daemon registry and Docker host path before its only mutation. This is not an atomic external race guarantee, only the currently established bounded precondition.

## Evidence boundary and next gate

The previous V2 overlay handoff/readback is accepted at user-terminal tier in [D4 LocalCache evidence](gatea-d4-localcache-root-identity-checkpoint-20261028.md): `--preflight`, `--prepare`, separate-process `--readback` all exit 0; 17 originals and seven overlay files hash-match. Original V2 root and D2 volume stay preserved.

**As of this admission:** NO current human-terminal volume creation response received; D4 output volume is `NOT_CREATED / UNVERIFIED` at this task's evidence tier, Docker volume execution `PENDING`, C6 native compile/link `NOT_RUN 0/2`. Gate A `PENDING/UNPROVEN`, Gate B `HOLD`, Stage4 `COLD`, library readiness `PRESERVED`. Even successful volume creation/readback will require a **separate explicit user approval** before any Docker container create/start or ESP-IDF native compile/link.
