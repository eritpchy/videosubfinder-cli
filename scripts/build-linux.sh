#!/bin/sh
set -eu
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/dist"
cmake --build build --parallel "${BUILD_JOBS:-4}"
ctest --test-dir build --output-on-failure
cmake --install build
python3 scripts/bundle.py dist
python3 tests/test_cli.py "$PWD/dist/VideoSubFinderCli.run"
