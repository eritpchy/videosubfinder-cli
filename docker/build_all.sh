#!/bin/bash
set -e
cd ${0%/*}
./build.sh
./build_cuda.sh