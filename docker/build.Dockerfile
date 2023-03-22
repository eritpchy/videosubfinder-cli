FROM ubuntu:20.04 as builder
# Allow ubuntu to cache package downloads
RUN rm -f /etc/apt/apt.conf.d/docker-clean
RUN --mount=type=cache,target=/var/cache/apt \
    apt update \
    && DEBIAN_FRONTEND=noninteractive apt install -y git build-essential libgtk-3-dev ffmpeg libavcodec-dev libavformat-dev \
      libavutil-dev libswscale-dev libx264-dev cmake libavcodec-dev libavformat-dev \
      libavutil-dev libswscale-dev libavfilter-dev libtbb-dev wget
ENV http_proxy http://192.168.50.86:10801
ENV https_proxy http://192.168.50.86:10801
ENV all_proxy http://192.168.50.86:10801
RUN mkdir -p /tmp/work \
    && cd /tmp/work \
    && git clone https://github.com/wxWidgets/wxWidgets.git \
    && cd wxWidgets/ \
    && git checkout v3.2.1 \
    && git submodule update --init --recursive \
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
    && cmake -DCMAKE_BUILD_TYPE=Release  -DWITH_GTK=OFF -DWITH_FFMPEG=ON -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local   -D WITH_TBB=ON -D WITH_V4L=ON -D WITH_OPENGL=ON \
    -D WITH_CUBLAS=ON -DWITH_QT=OFF -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" .. \
    && cmake --build . --config Release -j $(nproc) \
    && make install \
    && rm -rf /tmp/work/opencv-4.7.0 \
    && rm -f /tmp/work/4.7.0.tar.gz
COPY . /tmp/work/videosubfinder-src

RUN cd /tmp/work/videosubfinder-src \
    && rm -rf linux_build \
    && mkdir -p linux_build \
    && cd linux_build/ \
    && cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=OFF .. \
    && cmake --build . --config Release -j $(nproc) \
    && cp ./Interfaces/VideoSubFinderCli/VideoSubFinderCli /tmp/work/ \
    && rm -rf /tmp/work/videosubfinder-src
RUN cp -L /usr/local/lib/libwx_baseu-3.2.so.0 \
          /usr/local/lib/libopencv_videoio.so.407 \
          /usr/local/lib/libopencv_core.so.407 \
          /usr/local/lib/libopencv_imgproc.so.407 \
          /usr/local/lib/libopencv_imgcodecs.so.407 \
          /tmp/work/




















