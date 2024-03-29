FROM ubuntu:20.04 as builder
# Allow ubuntu to cache package downloads
RUN rm -f /etc/apt/apt.conf.d/docker-clean
RUN --mount=type=cache,target=/var/cache/apt,sharing=private \
    echo 1; apt update
RUN --mount=type=cache,target=/var/cache/apt,sharing=private \
    DEBIAN_FRONTEND=noninteractive apt install -y git cmake wget libtbb-dev \
      libavcodec-dev libgtk-3-dev libavformat-dev libswscale-dev libavfilter-dev build-essential \
    && if [[ "USE_GUI" = "1" ]] ; then DEBIAN_FRONTEND=noninteractive apt install -y \
        libgtk-3-dev ffmpeg libavutil-dev libx264-dev \
      ;fi
RUN mkdir -p /tmp/work \
    && cd /tmp/work \
    && git clone https://github.com/wxWidgets/wxWidgets.git \
    && cd wxWidgets/ \
    && git checkout v3.2.2.1 \
    && git submodule update --init --recursive \
    && mkdir buildgtk \
    && cd buildgtk/ \
    && ../configure --disable-gui \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/work/wxWidgets
RUN cd /tmp/work \
    && git clone https://github.com/opencv/opencv.git -b 4.8.0 --depth=1 \
    && cd opencv \
    && mkdir -p build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release  -DWITH_GTK=OFF -DWITH_FFMPEG=ON -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local   -D WITH_TBB=ON -D WITH_V4L=ON -D WITH_OPENGL=ON \
    -D WITH_CUBLAS=ON -DWITH_QT=OFF -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" .. \
    && cmake --build . --config Release -j $(nproc) \
    && make install \
    && rm -rf /tmp/work/opencv
