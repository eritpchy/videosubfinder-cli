#!/bin/sh
set -eu
release=armv7-eabihf--musl--stable-2024.05-1
checksum=5b34157a4c9f53064002f12f57c42836cd551c591a4be6a99b3ee201add506aa
export VSF_ARCH=armv7
export VSF_BUILD_ROOT=build/armv7
archive="$VSF_BUILD_ROOT/$release.tar.xz"
toolchain="$PWD/$VSF_BUILD_ROOT/toolchain"
mkdir -p "$toolchain"
if [ ! -x "$toolchain/bin/arm-buildroot-linux-musleabihf-g++" ]; then
    if ! echo "$checksum  $archive" | sha256sum -c - >/dev/null 2>&1; then
        curl -fL --connect-timeout 15 --max-time 600 --retry 3 \
            "https://toolchains.bootlin.com/downloads/releases/toolchains/armv7-eabihf/tarballs/$release.tar.xz" -o "$archive"
    fi
    echo "$checksum  $archive" | sha256sum -c -
    tar -xJf "$archive" --strip-components=1 -C "$toolchain"
fi
export VSF_CROSS_PREFIX="$toolchain/bin/arm-buildroot-linux-musleabihf-"
export VSF_STATIC_PREFIX="$PWD/$VSF_BUILD_ROOT/static-prefix"
export VSF_TEST_TIMEOUT=${VSF_TEST_TIMEOUT:-300}
sh scripts/build-static-linux.sh
