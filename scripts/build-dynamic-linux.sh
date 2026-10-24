#!/bin/sh
set -eu
jobs=${BUILD_JOBS:-4}
profile=$(. /etc/os-release; printf '%s%s' "$ID" "$VERSION_ID")
work="$PWD/build/dynamic-$profile"
prefix="$work/prefix"
revision=0654a42e19215ef25b1d367d822f3c630447e7c7
mkdir -p build/sources/opencv "$prefix"
if [ ! -f build/sources/opencv/.revision ]; then
    if [ -f source-dependencies/opencv.tar.gz ]; then
        tar -xzf source-dependencies/opencv.tar.gz --strip-components=1 -C build/sources/opencv
    else
        curl -fL --connect-timeout 15 --max-time 180 --retry 3 "https://codeload.github.com/opencv/opencv/tar.gz/$revision" | tar -xz --strip-components=1 -C build/sources/opencv
    fi
    printf '%s\n' "$revision" > build/sources/opencv/.revision
fi
test "$(cat build/sources/opencv/.revision)" = "$revision"
cmake -S build/sources/opencv -B "$work/opencv" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_LIST=core,imgproc,imgcodecs,videoio -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF \
    -DBUILD_JAVA=OFF -DBUILD_opencv_python3=OFF -DBUILD_opencv_apps=OFF \
    -DWITH_FFMPEG=ON -DOPENCV_FFMPEG_USE_FIND_PACKAGE=OFF \
    -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF -DWITH_QT=OFF -DWITH_V4L=OFF \
    -DWITH_IPP=OFF -DWITH_OPENCL=OFF -DWITH_TBB=OFF -DWITH_OPENEXR=OFF \
    -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_JASPER=OFF -DWITH_AVIF=OFF \
    -DWITH_OPENJPEG=OFF -DWITH_PROTOBUF=OFF -DBUILD_PROTOBUF=OFF -DWITH_ITT=OFF -DWITH_ADE=OFF -DBUILD_ITT=OFF \
    -DBUILD_ZLIB=ON -DBUILD_PNG=ON -DBUILD_JPEG=ON \
    -DVIDEOIO_ENABLE_PLUGINS=OFF -DCPU_DISPATCH=
cmake --build "$work/opencv" --parallel "$jobs"
cmake --install "$work/opencv"
cmake -S . -B "$work/cli" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DOpenCV_DIR="$prefix/lib/cmake/opencv4" -DCMAKE_INSTALL_PREFIX="$PWD/dist-dynamic"
cmake --build "$work/cli" --parallel "$jobs"
ctest --test-dir "$work/cli" --output-on-failure
cmake --install "$work/cli"
python3 scripts/bundle-worker.py "$prefix" dist-dynamic
python3 tests/test_cli.py "$PWD/dist-dynamic/VideoSubFinderCli.run"
