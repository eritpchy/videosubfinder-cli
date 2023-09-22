#!/bin/bash
set -e
cd ${0%/*}
./cpu/build.sh
./cpu_static/build.sh
./cuda/build_cuda.sh