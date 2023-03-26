FROM eritpchy/videosubfinder-build:base
COPY . /tmp/work/videosubfinder-src

RUN cd /tmp/work/videosubfinder-src \
    && cp -f ./Build/Linux_x64/VideoSubFinderCli.run /tmp/work/ \
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




















