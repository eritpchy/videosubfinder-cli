#!/bin/bash
set -e
cd ${0%/*}
if [[ "$GITHUB_ACTION" ]]; then
  docker buildx build --cache-from type=gha --cache-to type=gha,mode=max \
    -t videosubfinder-build:cpu-static -f build.Dockerfile ../../..
else
  docker build -t videosubfinder-build:cpu-static -f build.Dockerfile ../../..
fi
mkdir -p out
docker run --rm -v $PWD/out:$PWD/out videosubfinder-build:cpu-static \
  bash -c "ARCH=\$(uname -m) \
    && ARCH=\${ARCH/x86_64/x64} \
    && cd /tmp/work/ \
    && tar cvzf $PWD/out/videosubfinder-cli-cpu-static-linux-\$ARCH.tar.gz \
    VideoSubFinderCli VideoSubFinderCli.run settings \
    && mv -fv ./VideoSubFinderCli.upx ./VideoSubFinderCli \
    && tar cvzf $PWD/out/videosubfinder-cli-cpu-static-upx-linux-\$ARCH.tar.gz \
    VideoSubFinderCli VideoSubFinderCli.run settings \
  "