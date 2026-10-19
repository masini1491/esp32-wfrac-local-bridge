# D2-P1-T host negatives (synthetic only)

The existing command executes retained D1 36 and D2-P0 26 cases, then 14 additional
P1 named cases. D2 types now have TU-local linkage and an opt-in standalone-main
guard so reuse under strict `-Wall -Wextra -Werror` does not export anonymous D1
types. No existing assertion or inventory is removed. D1/P0 remain standalone.

```sh
python -B tests/host_cpp/run_deadline_d1.py --sdk /path/to/existing/pinned/esp-idf
# Existing CI command acquires only exact pinned parser sources:
python -B tests/host_cpp/run_deadline_d1.py --fetch-pinned-parser
```

The true pinned http_parser still participates in D1/P0. The P1 classes do not
include or execute an SDK TLS engine, sockets or threads. They encode the pinned
API's documented write/lifetime requirements as controlled stand-ins:

- WANT_READ and WANT_WRITE preserve exact buffer **address and length**. Equal
  bytes in a distinct array and same pointer with altered length are negative
  counterexamples, rejected before another fake call; only positive short-write
  counts advance the offset and remaining span. No second request is admitted.
- Config/CA/BIO alive flags must remain valid while fake SSL is live; deliberately
  invalidating each dependency blocks the next fake write. In-flight callback
  flags prevent early borrower/SSL disposal. No real dangling pointer is dereferenced.
- SSL-free or a successful close-notify flag does not create a socket-close
  acknowledgment. An injected void/masked close or unknown disposition quarantines
  the retained owner, blocks conflicting admission, and never permits a blind
  second close or a fabricated success witness. Successful disposal is once-only.
- Zero, negative, maximum/infinite and narrowing receive budgets are rejected;
  successive WANT iterations use the same decreasing budget and never dispatch
  a potentially-infinite zero-timeout receive. Synthetic milliseconds/capacities
  are test inputs, not device policy or hard caller-return claims.

Results are **SYNTHETIC_PASS**, not native pointer-lifetime, TLS verification,
handshake, socket leak, callback quiescence or cleanup-time evidence. The separate
`tests/espidf_probe` C3/C6 artifacts establish **SDK_COMPILE_ONLY** and report
actual port selection/config/symbol linkage; they are never executed. No production,
SDK, workflow, coordination, credential or firmware code changes are required.
Gate A stays PENDING/UNPROVEN, Gate B HOLD, Stage4 Cold; Library readiness PRESERVED.
