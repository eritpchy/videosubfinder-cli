#!/bin/bash
set -e
cd ${0%/*}
docker run --rm -v $PWD/../test_en.mp4:/test_en.mp4:ro -v $PWD/out:$PWD/out:ro alpine \
  sh -c "mkdir /tmp/work;cd /tmp/work/ && tar xvf $PWD/out/videosubfinder-cli-cpu-static-linux-\$(uname -m|sed 's|x86_64|x64|g').tar.gz \
    && time ./VideoSubFinderCli -i /test_en.mp4  -r -c -te 1 -be 0 -le 0 -re 1 \
    && (ls RGBImages/*.jpeg && echo TEST SUCCESS ||echo TEST FAIL) \
  "