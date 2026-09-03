#!/usr/bin/env bash
set -euo pipefail

if [[ ! -x build/bin/w100h ]]; then
  echo "W100h is not built. Run ./m first." >&2
  exit 1
fi

exec ./build/bin/w100h "$@"
