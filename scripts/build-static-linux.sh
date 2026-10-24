#!/bin/sh
set -eu
jobs=${BUILD_JOBS:-4}
build_root=${VSF_BUILD_ROOT:-build}
prefix="$PWD/$build_root/static-prefix"
mkdir -p "$build_root/sources" "$prefix"
fetch() {
    name=$1 repo=$2 revision=$3
    if [ ! -f "$build_root/sources/$name/.revision" ]; then
        mkdir -p "$build_root/sources/$name"
        if [ -f "source-dependencies/$name.tar.gz" ]; then
            tar -xzf "source-dependencies/$name.tar.gz" --strip-components=1 -C "$build_root/sources/$name"
        else
            curl -fL --connect-timeout 15 --max-time 300 --retry 3 "https://codeload.github.com/$repo/tar.gz/$revision" | tar -xz --strip-components=1 -C "$build_root/sources/$name"
        fi
        printf '%s\n' "$revision" > "$build_root/sources/$name/.revision"
    fi
    test "$(cat "$build_root/sources/$name/.revision")" = "$revision"
}
fetch ffmpeg FFmpeg/FFmpeg 894da5ca7d742e4429ffb2af534fcda0103ef593
fetch opencv opencv/opencv 0654a42e19215ef25b1d367d822f3c630447e7c7
if [ ! -f "$prefix/lib/libavformat.a" ]; then
    (cd "$build_root/sources/ffmpeg"
     set --
     if [ "${VSF_ARCH:-}" = armv7 ]; then
         set -- --enable-cross-compile --cross-prefix="$VSF_CROSS_PREFIX" \
             --arch=arm --cpu=armv7-a --target-os=linux --disable-neon \
             --extra-cflags='-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard'
     fi
     ./configure "$@" --prefix="$prefix" --disable-shared --enable-static --enable-pic \
        --disable-autodetect --disable-programs --disable-doc --disable-network \
        --disable-avdevice --disable-avfilter --disable-encoders --disable-muxers --disable-debug
     make -j"$jobs"
     make install)
fi
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
set --
if [ "${VSF_ARCH:-}" = armv7 ]; then
    export PKG_CONFIG_LIBDIR="$PKG_CONFIG_PATH"
    set -- -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/armv7-linux-musl.cmake" \
        -DCPU_BASELINE= -DENABLE_NEON=OFF
fi
cmake -S "$build_root/sources/opencv" -B "$build_root/opencv-static" -G Ninja "$@" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_LIST=core,imgproc,imgcodecs,videoio -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF \
    -DBUILD_JAVA=OFF -DBUILD_opencv_python3=OFF -DBUILD_opencv_apps=OFF \
    -DWITH_FFMPEG=ON -DOPENCV_FFMPEG_USE_FIND_PACKAGE=OFF \
    -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF -DWITH_QT=OFF -DWITH_V4L=OFF \
    -DWITH_IPP=OFF -DWITH_OPENCL=OFF -DWITH_TBB=OFF -DWITH_OPENEXR=OFF \
    -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_JASPER=OFF -DWITH_AVIF=OFF \
    -DWITH_OPENJPEG=OFF -DWITH_PROTOBUF=OFF -DBUILD_PROTOBUF=OFF -DWITH_ITT=OFF -DWITH_ADE=OFF -DBUILD_ITT=OFF \
    -DBUILD_ZLIB=ON -DBUILD_PNG=ON -DBUILD_JPEG=ON \
    -DVIDEOIO_ENABLE_PLUGINS=OFF -DCPU_DISPATCH= -DOPENCV_ENABLE_NONFREE=OFF
cmake --build "$build_root/opencv-static" --parallel "$jobs"
cmake --install "$build_root/opencv-static"
cmake -S . -B "$build_root/cli-static" -G Ninja "$@" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PWD/dist-static" -DOpenCV_DIR="$prefix/lib/cmake/opencv4" \
    -DCMAKE_EXE_LINKER_FLAGS='-static -static-libgcc -static-libstdc++'
cmake --build "$build_root/cli-static" --parallel "$jobs"
if [ "${VSF_ARCH:-}" = armv7 ]; then
    python3 scripts/check-binary.py "$build_root/cli-static/VideoSubFinderCli" armv7
fi
if readelf -l "$build_root/cli-static/VideoSubFinderCli" | grep -q INTERP; then
    echo 'The portable Linux binary must not require a dynamic loader' >&2
    exit 1
fi
ctest --test-dir "$build_root/cli-static" --output-on-failure
cmake --install "$build_root/cli-static"
python3 scripts/bundle.py dist-static
python3 tests/test_cli.py "$PWD/dist-static/VideoSubFinderCli.run"

mkdir -p dist-static/licenses/ffmpeg dist-static/licenses/opencv
cp $build_root/sources/ffmpeg/COPYING* dist-static/licenses/ffmpeg/
cp $build_root/sources/opencv/LICENSE dist-static/licenses/opencv/
cp -R $build_root/sources/opencv/3rdparty/libjpeg-turbo/LICENSE* dist-static/licenses/opencv/
