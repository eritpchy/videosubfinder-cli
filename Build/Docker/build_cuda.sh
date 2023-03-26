#!/bin/bash
set -e
cd ${0%/*}
if [[ "$GITHUB_ACTION" ]]; then
  docker buildx build --cache-from type=gha --cache-to type=gha,mode=max \
    -t videosubfinder-build:cuda -f build_cuda.Dockerfile ../..
else
  docker build -t videosubfinder-build:cuda -f build_cuda.Dockerfile ../..
fi
mkdir -p build/cuda/
docker run --rm -v $PWD/build/cuda/:$PWD/build/cuda/ videosubfinder-build:cuda \
  bash -c "cd /tmp/work/ && tar cvzf $PWD/build/cuda/videosubfinder-cli-cuda-linux-x64.tar.gz *"