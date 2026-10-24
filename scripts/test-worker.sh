#!/bin/sh
set -eu
name="vsf-worker-${GITHUB_RUN_ID:-local}-$$"
cleanup() { docker stop "$name" >/dev/null 2>&1 || true; }
trap cleanup EXIT INT TERM
docker run -d --rm --name "$name" -v "$PWD:$PWD" -w "$PWD" "$TEST_IMAGE" sleep 1200
docker exec "$name" sh -c 'apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ffmpeg'
export VSF_EXEC_PREFIX="docker exec --user $(id -u):$(id -g) --workdir {cwd} $name"
export VSF_TEST_MEDIA="$PWD/test-media"
python3 tests/test_cli.py "$PWD/dist-dynamic/VideoSubFinderCli.run"
