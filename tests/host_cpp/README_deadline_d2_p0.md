# D2-P0-T — controllable I/O and private-owner test-only witnesses

This is not an adapter, socket/TLS implementation, or Gate A feasibility PASS.
The current TASKS D2-P0-T contract is the authorization; this README grants none.
No production, SDK, firmware, workflow, coordination, or evidence file is changed.

## Reproducible execution and provenance

```sh
python -B tests/host_cpp/run_deadline_d1.py --sdk /path/to/existing/pinned/esp-idf
# Existing GitHub Host command (read-only parser source acquisition):
python -B tests/host_cpp/run_deadline_d1.py --fetch-pinned-parser
```

The retained runner now compiles `test_deadline_d2_p0.cpp`, which includes the
unchanged D1 fixtures with only the old standalone `main` excluded by a test-only
guard. All original 36 cases run first, with a fail-closed inventory assertion;
26 D2 cases follow. No parser code is copied or rewritten. The original D1 file
still compiles standalone. The runner hashes all three test/runner inputs and
checks exact upstream parser Git blobs before compilation as C99; the harness
is C++17, `-Wall -Wextra -Werror`. Compile/test subprocesses have a 60-second test
watchdog. A watchdog is test termination, not transport cancellation/quiescence.
Existing workflow commands/manifest hashes do not change. The new cases are
executed by the existing Host CI command, not a new workflow or SDK matrix.

Pinned sources: ESP-IDF v6.0.2
`7101770dc6db2667b3c477cc31365dd1acd6db4e`, esp-lwIP
`fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e`, mbedTLS
`6cc42afad309e861f4c07e6f106e2ab14a9cb8e5`.
Parser blobs remain `ef1ecd48c6aec122a1dbe132096fcda9713b964c` (C) and
`105ae510a8ab57509de2c68bbe1504e39ffb165e` (header), version 2.7.0.
Local offline validation can check all three SDK Git identities in the existing
pinned container, then link only its actual parser and project SchemePolicy.
Use `--network none`, source read-only, finite watchdog, and executable output
TMPDIR; no installed dependency/SDK modification or network device is needed.

## Coverage and deliberately limited semantics

- **A / nine named cases:** one checked shrinking budget across DNS/connect/
  readiness/TLS/short send/receive; delayed DNS or unpreemptible dispatch return;
  EINTR/EAGAIN/WANT_READ/WANT_WRITE; header/body trickle; zero/overflow/regression;
  separate I/O-completion and cleanup-return clocks. A frozen fake clock exhausts
  a finite script, not a purported autonomous timeout. Cleanup may overrun L1.
- **B / six cases:** actual D1 parser with malformed/partial/truncated/interim/
  chunked input, non-2xx atomic success, overflow scheme evidence, exact unsent
  bytes on TLS write retries, possibly delivered write without resend/downgrade,
  and primary-error preservation on uncertain cleanup. TCP-only is not an HTTPS
  handshake witness; untrusted overflow cannot become positive SchemePolicy
  evidence. Parser-stage data remains private.
- **C / seven cases:** explicit owner lifecycle, in-flight and late/duplicate/
  stale callbacks, executed fence versus queued fence, unknown close disposition,
  double-close/free exclusion and a real *synthetic admission gate* refusing a
  conflicting attempt. A single owner slot and two-generation fixture cap are
  test inputs, not production capacities. An absent callback stays quarantined.
  Callback contexts contain only generation/private-owner data, not caller-buffer
  pointers. A synthetic protocol waitstate flag is distinct from application
  ownership; it is not an actual FIN_WAIT/TIME_WAIT experiment.
- **D / four cases:** both schemes and original logical hostname/HTTP Host/SNI,
  required CA/name/SNI outcomes, rejection of numeric-address identity substitution,
  and no trusted channel without identity evidence. These are fake prerequisites,
  not real certificate verification, TLS BIO compatibility, or a handshake.

The A/B/C/D named counts above are grouped for navigation; the executable prints
the authoritative aggregate count (26). Some cases contain several counterexamples.
This is not a complete executable connection state machine: some parser/buffer
tests deliberately feed private data in isolation. Only publication and scheme
evidence require the explicit synthetic established-channel witness. Scripted
events/hints, timer, TLS outcomes, close acknowledgment, fence observation and
owner release are all controlled stand-ins, not imported native lifecycle facts.

## Evidence classes and remaining gaps

- Retained 19 D1 parser/policy cases execute the real pinned `http_parser`;
  retained 17 D1 policy cases and the 26 D2 integrated scenarios are
  `SYNTHETIC_PASS` only (even when the real parser participates).
- Source pins/parser identity are source provenance. No native socket, TLS/BIO,
  resolver, callback fence, cleanup or monotonic-clock SDK runtime is executed.
- Additional SDK API compile/link, A6b/T1/A7b replay and C3/C6 matrix are NOT_RUN:
  this candidate changes no platform/API/config dependency and needs Host CI only.
- Hostname never-callback recovery, real CA/hostname/SNI verification and BIO
  handoff, trusted failed-close/quiescence witnesses and actual resource timing
  remain UNPROVEN. No pending token/caller storage may be reclaimed by assuming
  that a synthetic fence, `cleanup()==OK` or `TEST_TERMINATION` proves safety.

Stage 3 stays sole Hot; Gate A PENDING/UNPROVEN, Gate B HOLD, Stage 4 Cold.
Library readiness PRESERVED. Real network/hardware/production validation NOT_RUN.
