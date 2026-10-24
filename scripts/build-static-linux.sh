#!/bin/sh
set -eu
jobs=${BUILD_JOBS:-4}
prefix="$PWD/build/static-prefix"
mkdir -p build/sources "$prefix"
fetch() {
    name=$1 repo=$2 revision=$3
    if [ ! -f "build/sources/$name/.revision" ]; then
        mkdir -p "build/sources/$name"
        if [ -f "source-dependencies/$name.tar.gz" ]; then
            tar -xzf "source-dependencies/$name.tar.gz" --strip-components=1 -C "build/sources/$name"
        else
            curl -fL --retry 3 "https://codeload.github.com/$repo/tar.gz/$revision" | tar -xz --strip-components=1 -C "build/sources/$name"
        fi
        printf '%s\n' "$revision" > "build/sources/$name/.revision"
    fi
    test "$(cat "build/sources/$name/.revision")" = "$revision"
}
fetch ffmpeg FFmpeg/FFmpeg 894da5ca7d742e4429ffb2af534fcda0103ef593
fetch opencv opencv/opencv 0654a42e19215ef25b1d367d822f3c630447e7c7
if [ ! -f "$prefix/lib/libavformat.a" ]; then
    (cd build/sources/ffmpeg
     ./configure --prefix="$prefix" --disable-shared --enable-static --enable-pic \
        --disable-autodetect --disable-programs --disable-doc --disable-network \
        --disable-avdevice --disable-avfilter --disable-encoders --disable-muxers --disable-debug
     make -j"$jobs"
     make install)
fi
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
cmake -S build/sources/opencv -B build/opencv-static -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_LIST=core,imgproc,imgcodecs,videoio -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF \
    -DBUILD_JAVA=OFF -DBUILD_opencv_python3=OFF -DBUILD_opencv_apps=OFF \
    -DWITH_FFMPEG=ON -DOPENCV_FFMPEG_USE_FIND_PACKAGE=OFF \
    -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF -DWITH_QT=OFF -DWITH_V4L=OFF \
    -DWITH_IPP=OFF -DWITH_OPENCL=OFF -DWITH_TBB=OFF -DWITH_OPENEXR=OFF \
    -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_JASPER=OFF -DWITH_AVIF=OFF \
    -DBUILD_ZLIB=ON -DBUILD_PNG=ON -DBUILD_JPEG=ON \
    -DVIDEOIO_ENABLE_PLUGINS=OFF -DCPU_DISPATCH= -DOPENCV_ENABLE_NONFREE=OFF
cmake --build build/opencv-static --parallel "$jobs"
cmake --install build/opencv-static
cmake -S . -B build/cli-static -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PWD/dist-static" -DOpenCV_DIR="$prefix/lib/cmake/opencv4" \
    -DCMAKE_EXE_LINKER_FLAGS='-static -static-libgcc -static-libstdc++'
cmake --build build/cli-static --parallel "$jobs"
if readelf -l build/cli-static/VideoSubFinderCli | grep -q INTERP; then
    echo 'The portable Linux binary must not require a dynamic loader' >&2
    exit 1
fi
ctest --test-dir build/cli-static --output-on-failure
cmake --install build/cli-static
python3 scripts/bundle.py dist-static
python3 tests/test_cli.py "$PWD/dist-static/VideoSubFinderCli.run"

mkdir -p dist-static/licenses/ffmpeg dist-static/licenses/opencv
cp build/sources/ffmpeg/COPYING* dist-static/licenses/ffmpeg/
cp build/sources/opencv/LICENSE dist-static/licenses/opencv/
cp -R build/sources/opencv/3rdparty/libjpeg-turbo/LICENSE* dist-static/licenses/opencv/
