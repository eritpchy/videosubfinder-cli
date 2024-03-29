FROM ubuntu:20.04 as builder
# Allow ubuntu to cache package downloads
RUN rm -f /etc/apt/apt.conf.d/docker-clean
ARG USE_GUI=0
RUN --mount=type=cache,target=/var/cache/apt,sharing=private \
    apt update
RUN --mount=type=cache,target=/var/cache/apt,sharing=private \
    DEBIAN_FRONTEND=noninteractive apt install -y ccache build-essential curl git
ENV PATH="/usr/lib/ccache:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/tmp/work/ffmpeg-build-script/workspace/bin"
RUN mkdir -p /tmp/work
RUN --mount=type=cache,target=/var/cache/apt,sharing=private \
    DEBIAN_FRONTEND=noninteractive apt install -y cmake
RUN --mount=type=cache,target=/root/.ccache,sharing=private \
    cd /tmp/work/ \
    && git clone https://github.com/markus-perl/ffmpeg-build-script.git -b v1.46 --depth=1 \
    && cd ffmpeg-build-script \
    && bash -c '([[ "aarch64" == "$(uname -m)" ]] && sed -i "s|https://github.com/videolan/x265/archive/Release_3.5.tar.gz|https://bitbucket.org/multicoreware/x265_git/get/931178347b3f73e40798fd5180209654536bbaa5.tar.gz|g" ./build-ffmpeg || true)' \
    && bash -c '([[ "aarch64" == "$(uname -m)" ]] && sed -i "s|https://github.com/georgmartius/vid.stab/archive/v1.1.0.tar.gz|https://github.com/meneguzzi/vid.stab/archive/refs/heads/sse2neon.tar.gz|g" ./build-ffmpeg || true)' \
    && sed -i "s|netactuate|onboardcloud|g" ./build-ffmpeg \
    && sed -i "s|netcologne|onboardcloud|g" ./build-ffmpeg \
    && AUTOINSTALL="yes" ./build-ffmpeg --enable-gpl-and-non-free --build --full-static \
    && true
RUN --mount=type=cache,target=/root/.ccache,sharing=private \
    cd /tmp/work \
    && git clone https://github.com/wxWidgets/wxWidgets.git -b v3.2.2.1 --depth=1 --recurse-submodules -j8 \
    && cd wxWidgets/ \
    && mkdir buildgtk \
    && cd buildgtk/ \
    && ../configure --disable-gui --disable-shared --disable-sys-libs \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/work/wxWidgets \
    && true 
RUN --mount=type=cache,target=/root/.ccache,sharing=private \
    cd /tmp/work \
    && git clone https://github.com/opencv/opencv.git -b 4.8.0 --depth=1 \
    && cd opencv \
    && mkdir -p build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release  -DWITH_GTK=OFF -DWITH_FFMPEG=ON -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local   -D WITH_TBB=ON -D WITH_V4L=ON -D WITH_OPENGL=ON \
    -D WITH_CUBLAS=ON -DWITH_QT=OFF -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES"  -DBUILD_SHARED_LIBS=OFF .. \
    && cmake --build . --config Release -j $(nproc) \
    && make install \
    && rm -rf /tmp/work/opencv \
    && true
# OpenCV full staitc library
RUN grep -R -l "\.so" /usr/local/lib/cmake/opencv4/*.cmake | xargs -I{} sed -i 's/\.so/.a/g' {}

RUN --mount=type=cache,target=/root/.ccache,sharing=private \
    cd /tmp/work \
    && git clone https://github.com/oneapi-src/oneTBB.git -b v2020.3.3 --depth=1 \
    && cd oneTBB \
    && make tbb_build_prefix=BUILDPREFIX extra_inc=big_iron.inc \
    && cp -f ./build/BUILDPREFIX_release/libtbb.a /usr/local/lib/ \
    && cp -f ./build/BUILDPREFIX_release/libtbbmalloc.a /usr/local/lib \
    && cp -rf ./include/tbb /usr/local/include/ \
    && rm -rf /tmp/work/oneTBB \
    && true
