FROM ubuntu
  
# install
RUN apt-get update && apt-get install -y \
    pwgen \
    g++ \
    g++-9 \
    make \
    cmake \
    libnuma-dev \
    wget \
    python3 \
    meson \
    udev \
    ninja-build \
    kmod \
    fuse3
# avoid prompt for info
RUN apt-get install --assume-yes git pip pkg-config
# python packages
RUN pip3 install meson pytest

WORKDIR /xsfs

# ouroboroslib setup
# get the specific supported commit
RUN git clone https://gitlab.com/xsearch/ouroboroslib/
WORKDIR ouroboroslib/build
RUN cmake ..
RUN make

WORKDIR ../..

# libfuse setup
RUN wget https://github.com/libfuse/libfuse/releases/download/fuse-3.10.5/fuse-3.10.5.tar.xz
RUN tar -xf fuse-3.10.5.tar.xz
WORKDIR fuse-3.10.5/build
RUN meson ..
RUN ninja install

WORKDIR ../..
COPY . .
WORKDIR ./build
RUN cmake ..
RUN make