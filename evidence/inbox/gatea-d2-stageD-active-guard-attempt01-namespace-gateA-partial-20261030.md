# GATEA-D2 Stage D active guard — attempt01 Gate A namespace probe PARTIAL (2026-10-30)

**Disposition: `ACTIVE_NAMESPACE_PARTIAL__EVIDENCE_GAP_STOPPED`.** This is a scoped review of the user-relayed Codex 13:54 Asia/Taipei report. GitHub verified the repository's remote `main` revision and task scope; no original supervisor stdout/stderr or raw tool trace was attached for independent byte-level review. Thus local kernel/namespace evidence is **Codex-reported**, not independently re-run by ChatGPT. Gate A is not accepted as a full isolation admission.

## Canonical context and non-mutation boundary

- Repository `masini1491/esp32-wfrac-local-bridge`; exact receiving revision `c9b3ac432395b2286313d26c1e81d2c5df695acc`, independently verified as remote `main` at review. Codex reports clean, correct linked worktree, safe `239453e4d41139ac5ae12a893f88df552815a9a0` → ff-only `c9b3ac432395b2286313d26c1e81d2c5df695acc`, and unchanged clean final worktree. Playbook `29d6cb3edd92ea9c899a2ec10c7ee4fa4bb7a70d`.
- Authorized child: `GATEA-D2-STAGED-WSL-ACTIVE-GUARD-CAPABILITY-NEGCTRL_TESTONLY_20261030`. Scope staged A → B → C → D, mandatory STOP if A cannot be fully corroborated, **no** target ELF. Prior read-only preflight child stays CLOSED.
- One Codex-reported 10-second bounded execution, approximate elapsed 0.037 seconds:
  `/usr/bin/unshare --user --map-root-user --mount --net --pid --fork --mount-proc --propagation private --kill-child=SIGKILL /usr/bin/python3 -B -c <sanitized child metadata reader>`
  Child raw wait status 0, normal exit 0, no signal; wrapper return code 0, stderr empty; no timeout. Original invocations/stdout/stderr reported as local tool record `da6a8e`, **not provided as an independently replayable artifact**.

## Codex-reported evidence

| Field | Parent → child / result | What it supports |
| --- | --- | --- |
| User namespace | `4026531837` → `4026532226` | A different reported user namespace inode |
| Mount namespace | `4026532218` → `4026532228` | A different reported mount namespace inode |
| PID namespace | `4026532221` → `4026532229` | A different reported PID namespace inode |
| Network namespace | `4026531833` → `4026532230` | A different reported network namespace inode |
| UID/GID mapping | namespace UID/GID 0 maps to original WSL user 1000 | Reported rootless user mapping, not host root |
| FD | `0=/dev/null`, `1/2=log pipes` | Child's own descriptor inventory only |
| Mount/proc | No shared mounts; proc has `nosuid,nodev,noexec` | Child's own observation, not independent host-supervisor cross-check |
| Network | Only `lo`, counters zero; empty IPv4 route; IPv6 only loopback entry | No observed non-loopback interfaces/routes; **no direct loopback interface flags** |
| Process state | Namespace-local capabilities remain; `NoNewPrivs=0`, `Seccomp=0` | Guard is **not** installed and capability-reduction proof is absent by design |

## Why STOP was required

The single permitted A probe ended before an independent, trusted supervisor inspected the **live** target child from outside its namespaces. Missing: bound host-PID ↔ namespace child identity, external `/proc/<hostPID>/ns/*`, `status`/`NSpid`, `uid_map`/`gid_map`, `fd` types and mount/network context cross-read while target was still alive; no independently checked raw child wait metadata. Child self-report alone is informative but not the specified external attestation. Loopback `flags` were not directly read; zero packet counters and empty routes do **not** establish that loopback is DOWN. No packet egress or network violation was demonstrated, but absence of evidence cannot be promoted to a zero-egress guarantee.

`NoNewPrivs=0`, `Seccomp=0` and namespace-local capabilities in a **pre-guard A capability probe** are expected incomplete enforcement state, not separate failure of namespace creation. They remain mandatory future guard implementation gates. This review **does not** establish that namespace creation failed; it records promising but insufficient **reported** capability evidence.

Per the admitted one-attempt / evidence-gap STOP contract, there was no repeat attempt. B guard implementation and C synthetic negative control execution are **NOT_RUN**, no guard/policy/probe code artifacts or hashes exist, and no target ELF was executed. This is **`PARTIAL`**, not a full Gate A `PASS`, and does not enable B or C by the original authorization.

## Specific next proposal (NOT authorized)

Request a **new explicit, narrowly bounded A-receipt corrective authorization** before any second active namespace attempt. Proposed scope: one more maximum 10-second, rootless and network-silent disposable namespace attempt, with an **outside trusted supervisor** that can identify and hold the specific child alive for inspection of `/proc/<hostPID>/ns/*`, `NSpid` and UID/GID mapping, `fd`/mount/route state, direct loopback `flags` (e.g. read-only `/sys/class/net/lo/flags` from within child context, with separately identified source), then release child and verify actual wait status. Use existing toolchain, enforce timeout + kill cleanup, capture bounded sanitized JSON/outside receipts and source provenance. The child must not be used as a trusted attestation source by itself. If supervisor cannot identify/hold child and correlate namespace IDs, or loopback state remains unproven, **STOP**, no retries, no scope expansion.

No real socket/connect/DNS syscalls, external network, sudo/setcap, package install, Docker/CI, ELF/runtime, seccomp filter installation, B/C implementation, Git source mutation, other worktree operations or permission workaround is proposed for this A-only corrective. Safe Git sync may be separately established at launch under ordinary project rules. After external evidence review and full A acceptance, only then consider whether remaining original B/C implementation authority can be invoked or needs reconciliation; **do not treat this note as automatic admission**.

**State preserved:** Sole Hot `GATEA-D2` with active child A halted at evidence gap; parent Gate A `PENDING/UNPROVEN`, Gate B `HOLD`, Stage 4 `COLD`, library readiness `PRESERVED`. L1/L2/L3 still UNPROVEN.
