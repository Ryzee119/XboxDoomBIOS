FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV CC="ccache clang"
ENV CXX="ccache clang++"
ENV ASM="ccache gcc"

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        nasm \
        cmake \
        ccache \
        python3 \
        python3-pip

RUN python3 -m pip install --upgrade pip && \
    pip install lz4 pyelftools

WORKDIR /workspace

CMD ["/bin/bash"]
