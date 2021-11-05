FROM ubuntu
  
# install
RUN apt-get update && apt-get install -y \
    pwgen \
    g++   \
    g++-9 \
    make  \
    cmake \
    libnuma-dev
# avoid prompt for infow
RUN apt-get install --assume-yes git

# WORKDIR /fuse-xsearch
# ouroboroslib setup
RUN git clone https://gitlab.com/xsearch/ouroboroslib
WORKDIR ouroboroslib/build
RUN g++-9 -v
RUN cmake ..
RUN make

# WORKDIR ../..