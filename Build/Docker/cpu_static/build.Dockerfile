FROM eritpchy/videosubfinder-build:base-cpu-static as builder
COPY . /tmp/work/videosubfinder-src
RUN cd /tmp/work/videosubfinder-src \
    && cp -rf ./Build/Linux_x64/* /tmp/work/ \
    && mkdir /tmp/work/settings && cp -rf ./Settings/general.cfg /tmp/work/settings/ \
    && rm -rf linux_build \
    && mkdir -p linux_build \
    && cd linux_build/ \
    && LD_LIBRARY_PATH=/tmp/work/ffmpeg-build-script/workspace/lib:/usr/local/lib:$LD_LIBRARY_PATH \
	   PKG_CONFIG_PATH=/tmp/work/ffmpeg-build-script/workspace/lib/pkgconfig:$PKG_CONFIG_PATH \
	   PKG_CONFIG_LIBDIR=/tmp/work/ffmpeg-build-script/workspace/lib:$PKG_CONFIG_LIBDIR \
	   cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=OFF \
	   		-DBUILD_FULL_STATIC=YES \
            -DFFMPEG_INCLUDE_DIRS=$(readlink -f /tmp/work/ffmpeg-build-script/workspace/include) \
            -DCMAKE_EXE_LINKER_FLAGS="-L/usr/lib/x86_64-linux-gnu -L/tmp/work/ffmpeg-build-script/workspace/lib" \
            .. \ 
    && cmake --build . --config Release -j $(nproc) \
    && cp ./Interfaces/VideoSubFinderCli/VideoSubFinderCli /tmp/work/ \
    && rm -rf /tmp/work/videosubfinder-src

RUN --mount=type=cache,target=/var/cache/apt \
    DEBIAN_FRONTEND=noninteractive apt install -y upx-ucl \
    && cd /tmp/work \
    && upx -f -o ./VideoSubFinderCli.upx ./VideoSubFinderCli \
    && true