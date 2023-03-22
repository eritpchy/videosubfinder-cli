#!/bin/bash
set -e
cd ${0%/*}
docker build -t videosubfinder-build:cpu -f build.Dockerfile ..
mkdir -p build/cpu/
docker run --rm -v $PWD/build/cpu/:$PWD/build/cpu/ videosubfinder-build:cpu \
  bash -c "cd /tmp/work/ && tar cvzf $PWD/build/cpu/VideoSubFinderCli.tar.gz *"