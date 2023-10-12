FROM eritpchy/videosubfinder-build:base
COPY . /tmp/work/videosubfinder-src
RUN cd /tmp/work/videosubfinder-src \
    && cp -rf ./Build/Linux_x64/* /tmp/work/ \
    && mkdir /tmp/work/settings && cp -rf ./Settings/general.cfg /tmp/work/settings/ \
    && rm -rf linux_build \
    && mkdir -p linux_build \
    && cd linux_build/ \
    && cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=OFF .. \
    && cmake --build . --config Release -j $(nproc) \
    && cp ./Interfaces/VideoSubFinderCli/VideoSubFinderCli /tmp/work/ \
    && rm -rf /tmp/work/videosubfinder-src
RUN cp -L /usr/local/lib/libwx_baseu-?.?.so.? \
          /usr/local/lib/libopencv_videoio.so.??? \
          /usr/local/lib/libopencv_core.so.??? \
          /usr/local/lib/libopencv_imgproc.so.??? \
          /usr/local/lib/libopencv_imgcodecs.so.??? \
          /tmp/work/




















