#!/bin/bash
set -e
cd ${0%/*}
docker build -t videosubfinder-build:cuda -f build_cuda.Dockerfile ..
mkdir -p build/cuda/
docker run --rm -v $PWD/build/cuda/:$PWD/build/cuda/ videosubfinder-build:cuda \
  bash -c "cd /tmp/work/ && tar cvzf $PWD/build/cuda/VideoSubFinderCli.tar.gz *"