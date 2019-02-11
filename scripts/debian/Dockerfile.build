ARG IMAGE_VERSION=latest
FROM debian:$IMAGE_VERSION
LABEL maintainer="DECENT"

# prerequisites
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    autotools-dev \
    automake \
    autoconf \
    libtool \
    make \
    cmake \
    g++ \
    flex \
    bison \
    doxygen \
    unzip \
    wget \
    git \
    qt5-default \
    qttools5-dev \
    qttools5-dev-tools \
    libreadline-dev \
    libcrypto++-dev \
    libgmp-dev \
    libssl1.0-dev \
    libboost-all-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    ca-certificates && \
    apt-get clean && apt-get autoremove && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# prepare directories
USER root
WORKDIR /root
RUN wget -nv https://sourceforge.net/projects/boost/files/boost/1.65.1/boost_1_65_1.tar.gz && \
    tar xvf boost_1_65_1.tar.gz && \
    cd boost_1_65_1 && \
    ./bootstrap.sh --prefix=../boost && \
    ./b2 install && \
    cd .. && \
    wget -nv https://cmake.org/files/v3.13/cmake-3.13.1.tar.gz && \
    tar xvf cmake-3.13.1.tar.gz && \
    cd cmake-3.13.1 && \
    ./configure --prefix=../cmake && \
    make install && \
    cd .. && \
    rm -rf boost_1_65_1* cmake-3.13.1* && \
    mkdir packages

# PBC
ARG PBC_VERSION=0.5.14
ARG PBC_GIT_REV=0.5.14
COPY pbc-build.sh .
RUN ./pbc-build.sh $PBC_VERSION $PBC_GIT_REV && \
    dpkg -i packages/libpbc* && \
    rm -rf pbc* packages/libpbc*

CMD ["/bin/bash"]
