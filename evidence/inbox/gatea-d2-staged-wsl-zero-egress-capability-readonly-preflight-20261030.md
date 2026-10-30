# GATEA-D2 Stage D — WSL zero-egress capability READ-ONLY preflight acceptance (2026-10-30)

**Disposition: `PREFLIGHT_READONLY_COMPLETED__CAPABILITIES_UNPROVEN__CHILD_CLOSED`.** This note records a Codex-reported local WSL passive inventory and a limited independent ChatGPT artifact comparison. Inventory acceptance is **not** a claim that effective zero-egress isolation, rootless namespaces, seccomp installation or runtime integration passed; there is **no** authorization to implement the guard or execute probes / target ELF.

## Provenance and exact boundary

- Project: `masini1491/esp32-wfrac-local-bridge`; target `main@239453e4d41139ac5ae12a893f88df552815a9a0` independently verified as GitHub remote at review time. Codex reports verified `main` linked worktree `C:\Users\user\Documents\esp32-wfrac-local-bridge-gatea-d2-clean-20261021`, safe `d3b93e982573896a87f900b813283d7b3738692e` → ff-only `239453e4d41139ac5ae12a893f88df552815a9a0`, no local ahead/divergence, clean final index/worktree. ChatGPT did **not** directly run local WSL Git commands; GitHub remote check confirms exact reported destination, not local clean status.
- Playbook `masini1491/ai-development-playbook@29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70d`. Governing Hot `TASKS.md` child `GATEA-D2-STAGED-WSL-ZERO-EGRESS-CAPABILITY-PREFLIGHT-READONLY_20261030`; initial Stage D design is `evidence/inbox/gatea-d2-stageD-zero-egress-enforcement-design-20261030.md`.
- Codex reports no namespace creation, seccomp filter installation, compile/build, synthetic control run, target ELF execution, Docker, downloads, package install, hardware/network operation or source mutation during inventory.

## Local observations (Codex-reported; not independently executed by ChatGPT)

| Check | Observation | Admissible interpretation |
| --- | --- | --- |
| Distro/kernel/WSL | Ubuntu 22.04.5 LTS; WSL `2.7.14.0`; `6.18.33.2-microsoft-standard-WSL2` x86-64 | Environment description only; actual isolation rights unknown |
| Shell process | UID/GID 1000:1000; CapEff/Prm/Inh/Amb 0; CapBnd `000001ffffffffff`; NoNewPrivs=0, Seccomp=0, Seccomp_filters=0 | Describes current shell, not future sandboxed child; no proof of filter-install rights |
| seccomp metadata | `/proc/sys/kernel/seccomp/actions_avail` contains `kill_process`, `user_notif`; read exit 0 | Kernel advertises actions, but unprivileged installation/verified policy unknown |
| User namespaces | `/proc/sys/user/max_user_namespaces=63779`; `/proc/sys/kernel/unprivileged_userns_clone` missing (read exit 1); namespace IDs observed but not compared | No demonstrated new user/net namespace creation or isolation |
| Bubblewrap | `bwrap` absent from PATH | Not a universal absence of sandbox capability; no installation authorized |
| Tools | `unshare` and `setpriv` util-linux 2.37.2, `capsh`/`getcap` libcap2 2.44, `strace` 5.16, `readelf` binutils 2.38, `file` 5.41, `sha256sum` coreutils 8.32, `python3` 3.10.12, `cc` GCC 11.4.0 present | Presence/version only; no side-effecting invocation; unsupported capsh/getcap help/version flags exit 1 do not imply absent tools |
| close_range | libc declaration and `close_range@@GLIBC_2.34`; x86-64 syscall no. 436 in headers | Actual effective kernel syscall support is UNKNOWN; not called |
| Target ELF | `/tmp/wfrac-httpclient-corrective-20261030-01/build/gatea_httpclient.elf`: ELF64 x86-64 PIE dynamic; SHA-256 `b95f16a87e5d0c53f54280653bf2a0a9bcafe58363d0170293fc5addde06a8c6` | Target not executed; separate local loader/libbsd/libc/libmd SHA-256s collected by Codex but not independently compared in this note |
| Dynamic loader | `/lib64/ld-linux-x86-64.so.2` resolved to `/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2`; Codex reports SHA-256 `41daf7836f7fce21a40ca63a00c5caf21b05a35a76ccc075d86f1716212cdf01`; no observed file capability | Current local hash unverified independently; subject to future policy/ELF dependency gate |

**Independent artifact confirmation by ChatGPT:** Reopened the exact previously uploaded `handoff(2).zip` in its read-only container environment. ZIP SHA-256 independently computed `9c0939179b81dbb35b8559a125d7a48612e939a87656c3bde93088ef51955460`; its `build/gatea_httpclient.elf` extracted **in-memory only** and recomputed SHA-256 `b95f16a87e5d0c53f54280653bf2a0a9bcafe58363d0170293fc5addde06a8c6`, 3,078,256 bytes, matching both the ZIP's `evidence/artifact-hashes.json` and Codex-reported local ELF hash. This crosses original handoff ↔ observed local ELF *by reported hash*, but ChatGPT did not directly inspect the local WSL file. No ELF execution or external networking occurred in the independent comparison.

## Scoped conclusion and remaining blockers

The bounded passive preflight is **COMPLETED and CLOSED at inventory-only tier**. Its status is neither `ZERO_EGRESS_ENFORCED` nor an acceptance of sandbox viability; an arbitrary WSL process with NoNewPrivs=0/Seccomp=0 is expected to be unrestricted and this does not in itself prove that an isolation launcher cannot establish restrictions on its child.

**Still UNPROVEN:** ability to create distinct rootless network/PID/user namespaces in the actual Codex WSL/sandbox context; apply independently attestable pre-exec seccomp-BPF with correct arch and default-kill semantics; reliably remove all inherited FD and dangerous env/privileges; install identical guard for synthetic negative controls and candidate; verify no attempted forbidden egress; maintain fail-closed timeout/receipt behavior. `bwrap` is currently absent; no install or replacement was authorized.

**Recommended next separately authorized step:** a *strictly scoped, test-only active guard capability and synthetic negative-control probe*, first verifying toolchain/namespace/filter functionality in a disposable environment with no target ELF. Any proposed probe must establish an outside supervisor and pre-exec deny boundary before unsafe syscall probes; no uncontrolled socket/DNS/network attempt, no production/SDK source mutation, no Docker/CI/downloads/package installation, and no inadvertent entry into runtime. ChatGPT must separately review receipts and any later target launch needs yet another explicit user approval. Do not launch active controls based on this record alone.

Sole Hot `GATEA-D2`; original corrective child `CORRECTIVE_SOURCE_COMMIT_PUSH_ACCEPTED__CHILD_CLOSED`; Gate A `PENDING/UNPROVEN`, Gate B `HOLD`, Stage 4 `COLD`, library readiness `PRESERVED`. No source/SDK/production change and no promotion of L1/L2/L3.
