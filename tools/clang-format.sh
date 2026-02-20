#!/usr/bin/env bash
set -euo pipefail

VER="18.1.8"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
  Linux)  OS_ID="linux" ;;
  Darwin) OS_ID="macos" ;;
  *) echo "Unsupported OS: $OS" >&2; exit 2 ;;
esac

case "$ARCH" in
  x86_64|amd64) ARCH_ID="x86_64" ;;
  arm64|aarch64) ARCH_ID="arm64" ;;
  *) echo "Unsupported arch: $ARCH" >&2; exit 2 ;;
esac

BIN="$ROOT/tools/clang-format-bin/$VER/$OS_ID-$ARCH_ID/clang-format"
if [[ ! -x "$BIN" ]]; then
  echo "Missing clang-format binary: $BIN" >&2
  exit 3
fi

# Fix Ubuntu-built clang-format runtime deps on Debian/Arch by vendoring needed libs
if [[ "$OS_ID" == "linux" ]]; then
  LIBDIR="$ROOT/tools/clang-format-bin/$VER/$OS_ID-$ARCH_ID/lib"
  if [[ -d "$LIBDIR" ]]; then
    export LD_LIBRARY_PATH="$LIBDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  fi
fi

exec "$BIN" "$@"
