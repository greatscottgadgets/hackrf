#!/bin/sh
set -eu

# Run from repo root (script is in repo root)
CF="./tools/clang-format.sh"

# Sanity check
if [ ! -x "$CF" ]; then
  echo "ERROR: $CF not found or not executable." >&2
  echo "Did you commit the pinned clang-format binaries and wrappers under tools/ ?" >&2
  exit 2
fi

# Print version for traceability
"$CF" --version

find host/libhackrf/src \
     host/hackrf-tools/src \
     firmware/common \
     firmware/hackrf_usb \
     \( -iname '*.h' -o -iname '*.hpp' -o -iname '*.c' -o -iname '*.cpp' \) \
     -print0 | \
     xargs -0 -r "$CF" -style=file -i
