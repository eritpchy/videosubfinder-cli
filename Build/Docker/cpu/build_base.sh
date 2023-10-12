#!/bin/bash
set -e
cd ${0%/*}
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  --push \
  -t eritpchy/videosubfinder-build:base \
  -f base.Dockerfile ../..
