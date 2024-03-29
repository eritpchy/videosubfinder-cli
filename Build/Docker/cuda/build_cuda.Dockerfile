FROM eritpchy/videosubfinder-build:base-cuda
COPY . /tmp/work/videosubfinder-src
RUN CUDA_DIR="$(ls -d1 /usr/local/cuda-*|head -1)" \
    && ln -s $CUDA_DIR/targets/x86_64-linux/lib/libcudart.so /usr/lib/libcudart.so \
    && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CUDA_DIR/lib64:$CUDA_DIR/extras/CUPTI/lib64 export PATH=$PATH:$CUDA_DIR/bin \
    && cd /tmp/work/videosubfinder-src \
    && cp -rf ./Build/Linux_x64/* /tmp/work/ \
    && rm -rf linux_build \
    && mkdir -p linux_build \
    && cd linux_build/ \
    && cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=ON .. \
    && cmake --build . --config Release -j $(nproc) \
    && if [[ "USE_GUI" = "1" ]] ; then cp ./Interfaces/VideoSubFinderWXW/VideoSubFinderWXW /tmp/work/; \
       else cp ./Interfaces/VideoSubFinderCli/VideoSubFinderCli /tmp/work/ ; fi \
    && rm -rf /tmp/work/videosubfinder-src
RUN cp -L /usr/local/lib/libwx_baseu-?.?.so.? \
          /usr/local/lib/libopencv_videoio.so.??? \
          /usr/local/lib/libopencv_core.so.??? \
          /usr/local/lib/libopencv_imgproc.so.??? \
          /usr/local/lib/libopencv_imgcodecs.so.??? \
          /tmp/work/















