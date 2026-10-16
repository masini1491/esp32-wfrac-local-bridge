#!/usr/bin/env bash
# No implicit profile or network acquisition. Unknown arguments fail closed.
set -eo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
exec python3 -B "$root/tools/local_ci.py" "$@"
