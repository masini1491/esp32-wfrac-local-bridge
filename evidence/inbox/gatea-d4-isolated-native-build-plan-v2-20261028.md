# GATEA-D4 — isolated native C6 manual-build V2 static plan (2026-10-28)

**Disposition:** `D4_ISOLATED_MANUAL_BUILD_PLAN_V2_STATIC_PREPARED__EXECUTION_HELD`. D4 preparation/provenance is actionable for **independent review only**, not a Docker volume/container permission or C6 build authorization. Same sole Hot `GATEA-D2`, same current child `GATEA-D4_PINNED_SDK_CUSTOM_TRANSPORT_COMPILE_LINK_ONLY_USER_AUTHORIZED_20261027`. Stage-local D4 native attempts **0/2 consumed**. Original static preparation accepted, not reopened.

## Source material and verified environment

- User-uploaded `d4-preparation-handoff.zip`: independently accepted static-only SHA-256 `dd1ccb230d4abe19aa95169b88d341e0beda0a4c0b48208a6a9d05088da74a2f`. The original root marker, `build-c6.sh` SHA-256 `b83393f602323e16e531bf5f26c285971f9989ceea0135cca6801fb9fa12f7d9`, seven pinned `build-inputs.sha256` entries and `fixture/` are **not edited**. New V2 resides as a proposed separate `d4-v2/` overlay only, not materialized in the user's Windows root by ChatGPT.
- User's in-image read-only checks previously verified Docker image `sha256:0d8c9773d48a327233f9c1d7c654ff0bcf133ae24503ea2e97a57cfe02b8cb67` (linux/amd64), IDF `7101770dc6db2667b3c477cc31365dd1acd6db4e`, lwIP `fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`, 5 pinned SDK blobs; exactly one compiler at `/opt/esp/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin/riscv32-esp-elf-g++`, version crosstool-NG `esp-15.2.0_20251204`, observed binary SHA-256 `a0dfebec55dd9c6819cd554262c8d1dee784a2a0724af134ebdbc9190aa763aa`. No compiled C6 ELF/map is yet proven; SHA is local-observed, not a separately published per-binary checksum attestation.

## Newly generated independent V2 review artifacts

Created in ChatGPT working container only; **not written to the user's Windows root, Docker daemon or repository production source**:

- `d4-isolated-build-review-v2-20261028.zip`, **11,096 bytes**, SHA-256 **`d85e863c0ea97961682b5c381b54f35420a2eb57f13b6ffc4d35a72c6143bd70`**. **7 distinct ZIP members**, **6 manifest payloads**, SHA/byte/CRC verified. The whole overlay is under prefix `d4-v2/`: versioned `build-c6-isolated.sh`, `MANUAL_BUILD_HELD_V2.md`, `revision-contract.json`, `README.md`, `build-script-diff.patch`, `review-inputs.sha256` plus the self-excluded `package-manifest.json`. None of the historical root-level file names is replaced.
- `d4-isolated-build-v2-static-audit-20261028.json`, SHA-256 **`a5f98145eedd63c306247f73bfaf6ff31e9d2796e0ec3efa704fa0bc791df12c`**, records **24/24** bounded independent static/archive/guard checks PASS. These checks are not C6 SDK build tests and did not execute Docker/compiler/fixture code.
- Original-to-V2 `build-script-diff.patch` independently reproduced byte-for-byte; `bash -n` on new versioned script PASS. No Git-tracked production files or former stage evidence changed. Project coordination document is the only repository mutation in this approval.

## Main revision and safeguards

**Isolation:** Future Docker container mount whitelist **exactly** `/src` (existing validated D4 preparation root bind **RO**) and `/out` (proposed **separate D4-only Docker volume** `wfrac-gatea-d4-c6-20261028-01`, RW); no D2 historical volume mount of any mode. **Protected D2 volume `wfrac-gatea-d2-fs-20261021-01` is NEVER to be mounted by D4**, and the historical manual build command that mounted it RW stays **HELD/UNSAFE TO EXECUTE**. A Docker `local` volume driver alone does not attest ext4; prove actual backing filesystem/disjointness before representing it as ext4.

**Source/pin preservation:** Separate V2 script checks unchanged original seven source inputs, V2 overlay source hashes, exact IDF/lwIP HEAD, SDK content and `export.sh` blob, and SDK tracked status. The previously located C6 compiler path and observed SHA are checked both before and after activating the SDK environment; fail-closed on selecting another compiler. With a future explicit approval, generate an inert C6 fixture ELF/map and collect linker commands, consumer object relocations, symbols and sources before/after; **never execute the ELF**. No runtime/TLS/DNS/deadline/cleanup evidence will follow from compile/link alone.

**Authorizations still separate and missing:**
1. Independent archive/source review complete for this turn, but **user has NOT yet authorized any manual transfer/copy of V2 overlay into the existing Windows root**; original marker and fixture byte/hash verification and destination absence must be confirmed before any such copy.
2. D4-only volume **NOT CREATED**. Requires dedicated user consent to `docker volume create` with verified absent name and safe filesystem/disjointness; no attempt to reuse existing D4 volume automatically.
3. Docker `create` / `start -a` and native C6 compile/link **NOT AUTHORIZED**. Literal manual command in `MANUAL_BUILD_HELD_V2.md` is review-only, not to be pasted/executed. Container pre-start inspection must confirm image/entrypoint/args/network/read-only/capabilities/mount whitelist, especially **NO D2 mount**. After exact separate user approval, one C6 compile/link aggregate maximum 1200 sec; a second only for evidenced narrow fixture/toolchain failure. Preserve all outcomes/returns; no retry beyond 2/2.

**Remaining technical uncertainties:** Root/DrvFS bind availability, new volume native ext4 identity, exact SDK activation behavior, actual custom-transport C++ compile/link, linker retention/map provenance, Docker volume output export behavior. SDK's actual physical deadline/callback quiescence/TLS/DNS cleanup remain `UNPROVEN`.

**State:** `D4_ISOLATED_MANUAL_BUILD_PLAN_V2_STATIC_PREPARED__EXECUTION_HELD`, C6 native compile/link **NOT_RUN**, attempts **0/2**, D3 model-only CLOSED (3/3 exhausted), Stage2R trace-limited CLOSED, Gate A PENDING/UNPROVEN, Gate B HOLD, Stage4 COLD, library readiness PRESERVED.
