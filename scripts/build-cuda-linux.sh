#!/bin/sh
set -eu
# Build static media dependencies on glibc; CUDA/NPP remain dynamically linked.
# Run from the repository root in nvidia/cuda:12.6.3-devel-ubuntu22.04.
jobs=${BUILD_JOBS:-4}
export CMAKE_BUILD_PARALLEL_LEVEL="$jobs"
prefix="$PWD/build/cuda-prefix"
mkdir -p build/sources "$prefix"
fetch() {
    name=$1 repo=$2 revision=$3
    if [ ! -f "build/sources/$name/.revision" ]; then
        mkdir -p "build/sources/$name"
        if [ -f "source-dependencies/$name.tar.gz" ]; then
            tar -xzf "source-dependencies/$name.tar.gz" --strip-components=1 -C "build/sources/$name"
        else
            curl -fL --connect-timeout 15 --max-time 300 --retry 3 "https://codeload.github.com/$repo/tar.gz/$revision" | tar -xz --strip-components=1 -C "build/sources/$name"
        fi
        printf '%s\n' "$revision" > "build/sources/$name/.revision"
    fi
    test "$(cat "build/sources/$name/.revision")" = "$revision"
}
fetch dav1d videolan/dav1d 54706fc6bc0cdecab7e9593974a4039cc038fca7
fetch ffmpeg FFmpeg/FFmpeg 894da5ca7d742e4429ffb2af534fcda0103ef593
fetch opencv opencv/opencv 0654a42e19215ef25b1d367d822f3c630447e7c7
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
# Do not let system ffmpeg (installed solely for test fixtures) leak into linkage.
export PKG_CONFIG_LIBDIR="$prefix/lib/pkgconfig"
if [ ! -f "$prefix/lib/libdav1d.a" ]; then
    meson setup build/dav1d-cuda build/sources/dav1d --prefix="$prefix" --libdir=lib \
        --buildtype=release --default-library=static -Db_staticpic=true \
        -Denable_tools=false -Denable_tests=false
    meson compile -C build/dav1d-cuda -j "$jobs"
    meson install -C build/dav1d-cuda
fi
if [ ! -f "$prefix/lib/libavformat.a" ] || [ ! -f "$prefix/.vsf-dav1d-1.5.4" ]; then
    (cd build/sources/ffmpeg
     ./configure --prefix="$prefix" --disable-shared --enable-static --enable-pic \
        --disable-autodetect --enable-libdav1d --pkg-config-flags=--static --disable-programs --disable-doc --disable-network \
        --disable-avdevice --disable-avfilter --disable-encoders --disable-muxers --disable-debug
     make -j"$jobs"
     make install)
    touch "$prefix/.vsf-dav1d-1.5.4"
fi
cmake -S build/sources/opencv -B build/opencv-cuda -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_LIST=core,imgproc,imgcodecs,videoio -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF \
    -DBUILD_JAVA=OFF -DBUILD_opencv_python3=OFF -DBUILD_opencv_apps=OFF \
    -DWITH_CUDA=OFF -DWITH_GDAL=OFF -DWITH_GDCM=OFF -DWITH_FFMPEG=ON -DOPENCV_FFMPEG_USE_FIND_PACKAGE=OFF -DPKG_CONFIG_ARGN=--static \
    -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF -DWITH_QT=OFF -DWITH_V4L=OFF \
    -DWITH_IPP=OFF -DWITH_OPENCL=OFF -DWITH_TBB=OFF -DWITH_OPENEXR=OFF \
    -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_JASPER=OFF -DWITH_AVIF=OFF \
    -DWITH_OPENJPEG=OFF -DWITH_PROTOBUF=OFF -DBUILD_PROTOBUF=OFF -DWITH_ITT=OFF -DWITH_ADE=OFF -DBUILD_ITT=OFF \
    -DBUILD_ZLIB=ON -DBUILD_PNG=ON -DBUILD_JPEG=ON \
    -DVIDEOIO_ENABLE_PLUGINS=OFF -DCPU_DISPATCH= -DOPENCV_ENABLE_NONFREE=OFF
cmake --build build/opencv-cuda --parallel "$jobs"
cmake --install build/opencv-cuda
cmake -S . -B build/cli-cuda -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PWD/dist-cuda" -DOpenCV_DIR="$prefix/lib/cmake/opencv4" \
    -DUSE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES='61;75;80;86;89;90'
cmake --build build/cli-cuda --parallel "$jobs"
ctest --test-dir build/cli-cuda --output-on-failure
cmake --install build/cli-cuda --strip
python3 scripts/bundle.py dist-cuda
python3 tests/test_cli.py "$PWD/dist-cuda/VideoSubFinderCli.run"

mkdir -p dist-cuda/licenses/ffmpeg dist-cuda/licenses/opencv
cp build/sources/ffmpeg/COPYING* dist-cuda/licenses/ffmpeg/
cp build/sources/dav1d/COPYING dist-cuda/licenses/dav1d-COPYING
cp build/sources/opencv/LICENSE dist-cuda/licenses/opencv/
cp -R build/sources/opencv/3rdparty/libjpeg-turbo/LICENSE* dist-cuda/licenses/opencv/
for dependency in libpng zlib; do
    mkdir -p "dist-cuda/licenses/opencv/$dependency"
    for notice in build/sources/opencv/3rdparty/"$dependency"/LICENSE* build/sources/opencv/3rdparty/"$dependency"/README*; do
        [ ! -f "$notice" ] || cp "$notice" "dist-cuda/licenses/opencv/$dependency/"
    done
done

# Preserve toolkit terms even when the official devel image omits EULA.txt.
python3 scripts/preserve-cuda-license.py --cuda-root "${CUDA_HOME:-/usr/local/cuda}" --output dist-cuda/licenses/cuda
