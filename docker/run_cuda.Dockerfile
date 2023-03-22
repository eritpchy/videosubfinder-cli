FROM nvidia/cuda:11.7.0-runtime-ubuntu20.04 as builder
# Allow ubuntu to cache package downloads
RUN rm -f /etc/apt/apt.conf.d/docker-clean
ARG USE_GUI=0
RUN --mount=type=cache,target=/var/cache/apt \
    apt update \
    && DEBIAN_FRONTEND=noninteractive apt install -y libavcodec58 \
    libavformat58 libswscale5 libavfilter7 libpcre2-32-0 libtbb2
ADD build/cuda/VideoSubFinderCli.tar.gz /usr/lib
RUN ls /usr/lib -l && mv /usr/lib/VideoSubFinderCli /