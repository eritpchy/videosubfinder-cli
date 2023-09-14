#!/bin/bash
set -e
cd ${0%/*}
docker build -t eritpchy/videosubfinder-build:base -f base.Dockerfile ../..
docker push eritpchy/videosubfinder-build:base