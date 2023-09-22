#!/bin/bash
set -e
cd ${0%/*}
if [[ "$GITHUB_ACTION" ]]; then
  docker buildx build --cache-from type=gha --cache-to type=gha,mode=max \
    -t videosubfinder-build:cuda -f build_cuda.Dockerfile ../../..
else
  docker build -t videosubfinder-build:cuda -f build_cuda.Dockerfile ../../..
fi
mkdir -p out
docker run --rm -v $PWD/out:$PWD/out videosubfinder-build:cuda \
  bash -c "cd /tmp/work/ && tar cvzf $PWD/out/videosubfinder-cli-cuda-linux-x64.tar.gz *"