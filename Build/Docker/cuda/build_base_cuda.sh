#!/bin/bash
set -e
cd ${0%/*}
docker build -t eritpchy/videosubfinder-build:base-cuda -f base_cuda.Dockerfile ../..
docker push eritpchy/videosubfinder-build:base-cuda