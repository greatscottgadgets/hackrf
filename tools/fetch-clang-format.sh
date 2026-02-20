#!/usr/bin/env bash
set -euo pipefail

VER="18.1.8"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$ROOT/tools/clang-format-bin/$VER"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

base="https://github.com/llvm/llvm-project/releases/download/llvmorg-${VER}"

# Archives you said you want to support
LINUX_X86="clang+llvm-${VER}-x86_64-linux-gnu-ubuntu-18.04.tar.xz"
LINUX_A64="clang+llvm-${VER}-aarch64-linux-gnu.tar.xz"
MAC_A64="clang+llvm-${VER}-arm64-apple-macos11.tar.xz"
WIN_X86="clang+llvm-${VER}-x86_64-pc-windows-msvc.tar.xz"

mkdir -p \
  "$DEST/linux-x86_64" \
  "$DEST/linux-arm64" \
  "$DEST/macos-arm64" \
  "$DEST/windows-x86_64"

fetch_and_extract() {
  local archive="$1"
  local outpath="$2"
  local inner="$3"

  echo "Downloading $archive"
  curl -fsSL "$base/$archive" -o "$TMP/$archive"

  echo "Extracting $inner"
  tar -xf "$TMP/$archive" -C "$TMP"

  # The tarball extracts into a single top-level directory; find it
  local topdir
  topdir="$(find "$TMP" -maxdepth 1 -type d -name "clang+llvm-${VER}-*" | head -n 1)"
  if [[ -z "${topdir:-}" ]]; then
    echo "Could not locate extracted directory for $archive" >&2
    exit 4
  fi

  if [[ ! -f "$topdir/$inner" ]]; then
    echo "Missing expected file inside archive: $inner" >&2
    exit 5
  fi

  cp "$topdir/$inner" "$outpath"
  rm -rf "$topdir"
}

# Linux x86_64 -> clang-format
fetch_and_extract "$LINUX_X86" "$DEST/linux-x86_64/clang-format" "bin/clang-format"
chmod +x "$DEST/linux-x86_64/clang-format"

# Linux arm64 -> clang-format
fetch_and_extract "$LINUX_A64" "$DEST/linux-arm64/clang-format" "bin/clang-format"
chmod +x "$DEST/linux-arm64/clang-format"

# macOS arm64 -> clang-format
fetch_and_extract "$MAC_A64" "$DEST/macos-arm64/clang-format" "bin/clang-format"
chmod +x "$DEST/macos-arm64/clang-format"

# Windows x86_64 -> clang-format.exe
fetch_and_extract "$WIN_X86" "$DEST/windows-x86_64/clang-format.exe" "bin/clang-format.exe"

echo
echo "Done. Installed pinned clang-format binaries under:"
echo "  $DEST"
echo
echo "Verify:"
"$ROOT/tools/clang-format" --version || true

