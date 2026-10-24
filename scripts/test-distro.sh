#!/bin/sh
set -eu
bundle/VideoSubFinderCli.run --version
APPIMAGE_EXTRACT_AND_RUN=1 packages/videosubfinder-cli-linux-${VSF_ARCH:-x64}.AppImage --version
if command -v apt-get >/dev/null; then
    apt-get update -qq
    DEBIAN_FRONTEND=noninteractive apt-get install -y python3
elif command -v dnf >/dev/null; then
    dnf -y install python3
elif command -v zypper >/dev/null; then
    zypper --non-interactive install python3
else
    pacman -Syu --noconfirm python
fi
VSF_TEST_MEDIA="$PWD/test-media" python3 tests/test_cli.py "$PWD/bundle/VideoSubFinderCli.run"
VSF_TEST_MEDIA="$PWD/test-media" APPIMAGE_EXTRACT_AND_RUN=1 python3 tests/test_cli.py "$PWD/packages/videosubfinder-cli-linux-${VSF_ARCH:-x64}.AppImage"
