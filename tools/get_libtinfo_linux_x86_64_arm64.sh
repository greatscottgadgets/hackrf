#!/usr/bin/env bash
set -euo pipefail

VER="18.1.8"
X86_DEST="clang-format-bin/${VER}/linux-x86_64/lib"
ARM_DEST="clang-format-bin/${VER}/linux-arm64/lib"

mkdir -p "$X86_DEST" "$ARM_DEST"

docker run --rm \
  --platform linux/amd64 \
  -v "$PWD":/repo \
  ubuntu:18.04 \
  bash -lc "
    set -euo pipefail
    export DEBIAN_FRONTEND=noninteractive

    apt-get update
    apt-get install -y --no-install-recommends ca-certificates dpkg-dev curl

    mkdir -p /tmp/dl /tmp/t
    cd /tmp/dl

    echo '[*] amd64: download + extract libtinfo5'
    apt-get download libtinfo5
    rm -rf /tmp/t && mkdir -p /tmp/t
    dpkg-deb -x /tmp/dl/libtinfo5_*_amd64.deb /tmp/t
    cp -av /tmp/t/lib/x86_64-linux-gnu/libtinfo.so.5* /repo/${X86_DEST}/

    echo '[*] arm64: download libtinfo5 .deb directly + extract'
    # This is the Ubuntu 18.04 (bionic-updates) arm64 libtinfo5 package matching the amd64 one above.
    ARM_DEB='libtinfo5_6.1-1ubuntu1.18.04.1_arm64.deb'
    ARM_URL='http://ports.ubuntu.com/ubuntu-ports/pool/main/n/ncurses/'\"\$ARM_DEB\"

    curl -fsSL \"\$ARM_URL\" -o \"/tmp/dl/\$ARM_DEB\"

    rm -rf /tmp/t && mkdir -p /tmp/t
    dpkg-deb -x \"/tmp/dl/\$ARM_DEB\" /tmp/t
    cp -av /tmp/t/lib/aarch64-linux-gnu/libtinfo.so.5* /repo/${ARM_DEST}/

    echo
    echo '[*] Done. Repo now contains:'
    ls -la /repo/${X86_DEST} || true
    ls -la /repo/${ARM_DEST} || true
  "
