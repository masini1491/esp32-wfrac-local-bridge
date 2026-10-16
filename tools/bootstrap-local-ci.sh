#!/usr/bin/env bash
# Non-installing preflight only; no fetch, pull, login, service or group changes.
set -eo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
command -v python3 >/dev/null || { echo 'TOOLCHAIN: Python 3 missing; request approved setup' >&2; exit 1; }
exec python3 -B "$root/tools/local_ci.py" --check "$@"
