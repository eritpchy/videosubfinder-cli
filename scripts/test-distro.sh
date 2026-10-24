#!/bin/sh
set -eu
name="vsf-distro-${GITHUB_RUN_ID:-local}-$$"
cleanup() { docker stop "$name" >/dev/null 2>&1 || true; }
trap cleanup EXIT INT TERM
mkdir -p build/test-work
# Host Python drives tests so EOL guest package mirrors and Python versions are irrelevant.
set --
if [ -n "${VSF_DOCKER_PLATFORM:-}" ]; then
    set -- --platform "$VSF_DOCKER_PLATFORM"
fi
docker run "$@" -d --rm --name "$name" --user "$(id -u):$(id -g)" \
    -e APPIMAGE_EXTRACT_AND_RUN=1 -v "$PWD:$PWD" -w "$PWD" \
    "$TEST_IMAGE" sh -c 'sleep 1200'
export VSF_EXEC_PREFIX="docker exec --workdir {cwd} $name"
export VSF_TEST_MEDIA="$PWD/test-media"
python3 tests/test_cli.py "$PWD/bundle/VideoSubFinderCli.run"
python3 tests/test_cli.py "$PWD/packages/videosubfinder-cli-linux-${VSF_ARCH:-x64}.AppImage"
