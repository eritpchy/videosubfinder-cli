FROM nvidia/cuda:11.7.0-devel-ubuntu20.04 as builder
# Allow ubuntu to cache package downloads
RUN rm -f /etc/apt/apt.conf.d/docker-clean
ARG USE_GUI=0
RUN --mount=type=cache,target=/var/cache/apt \
    apt update
RUN --mount=type=cache,target=/var/cache/apt \
    DEBIAN_FRONTEND=noninteractive apt install -y git cmake wget libtbb-dev \
      libavcodec-dev libgtk-3-dev libavformat-dev libswscale-dev libavfilter-dev \
    && if [[ "USE_GUI" = "1" ]] ; then DEBIAN_FRONTEND=noninteractive apt install -y \
        build-essential libgtk-3-dev ffmpeg libavutil-dev libx264-dev \
      ;fi
RUN mkdir -p /tmp/work \
    && cd /tmp/work \
    && git clone https://github.com/wxWidgets/wxWidgets.git --branch v3.2.1 --depth=1 --recurse-submodules -j8 \
    && cd wxWidgets/ \
    && mkdir buildgtk \
    && cd buildgtk/ \
    && ../configure --disable-gui \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/work/wxWidgets
RUN cd /tmp/work \
    && wget https://github.com/opencv/opencv/archive/4.7.0.tar.gz \
    && tar xvf 4.7.0.tar.gz \
    && cd opencv-4.7.0/ \
    && mkdir -p build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DWITH_GTK=OFF -DWITH_FFMPEG=ON -D CMAKE_BUILD_TYPE=RELEASE \
       -D CMAKE_INSTALL_PREFIX=/usr/local   -D WITH_TBB=ON -D WITH_V4L=ON -D WITH_OPENGL=ON \
       -D WITH_CUBLAS=ON -DWITH_QT=OFF -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" .. \
    && cmake --build . --config Release -j $(nproc) \
    && make install \
    && rm -rf /tmp/work/opencv-4.7.0 \
    && rm -f /tmp/work/4.7.0.tar.gz