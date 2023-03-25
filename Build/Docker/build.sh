#!/bin/bash
set -e
cd ${0%/*}
if [[ "$GITHUB_ACTION" ]]; then
  docker buildx build --cache-from type=gha --cache-to type=gha,mode=max \
    -t videosubfinder-build:cpu -f build.Dockerfile ../..
else
  docker build -t videosubfinder-build:cpu -f build.Dockerfile ../..
fi
mkdir -p build/cpu/
docker run --rm -v $PWD/build/cpu/:$PWD/build/cpu/ videosubfinder-build:cpu \
  bash -c "cd /tmp/work/ && tar cvzf $PWD/build/cpu/videosubfinder-cli-cpu-linux-x64.tar.gz *"