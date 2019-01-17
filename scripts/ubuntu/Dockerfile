ARG IMAGE_VERSION=latest
FROM ubuntu:$IMAGE_VERSION
LABEL maintainer="DECENT"

# prerequisites
ARG IMAGE_VERSION
RUN apt-get update && \
    if [ "$IMAGE_VERSION" != "16.04" ]; then apt-get install -y --no-install-recommends \
        libreadline7 \
        libcrypto++6 \
        libssl1.1 \
        libcurl4 \
        wget \
        ca-certificates; \
    else apt-get install -y --no-install-recommends \
        libreadline6 \
        libcrypto++9v5 \
        libssl1.0.0 \
        libcurl3 \
        wget \
        ca-certificates; \
    fi && apt-get clean && apt-get autoremove

# PBC
ARG PBC_VERSION=0.5.14
RUN wget -P /tmp https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc_$PBC_VERSION-ubuntu${IMAGE_VERSION}_amd64.deb && \
    dpkg -i /tmp/libpbc_$PBC_VERSION-ubuntu${IMAGE_VERSION}_amd64.deb && \
    rm /tmp/libpbc_$PBC_VERSION-ubuntu${IMAGE_VERSION}_amd64.deb

# IPFS
ARG IPFS_VERSION=v0.4.18
ARG TARGETOS=linux
ARG TARGETARCH=amd64
RUN wget -P /tmp "https://dist.ipfs.io/go-ipfs/${IPFS_VERSION}/go-ipfs_${IPFS_VERSION}_${TARGETOS}-${TARGETARCH}.tar.gz" && \
    cd /tmp && tar xvf "go-ipfs_${IPFS_VERSION}_${TARGETOS}-${TARGETARCH}.tar.gz" && \
    cp go-ipfs/ipfs /usr/bin && \
    rm -rf go-ipfs*

# prepare directories
ENV DCORE_HOME=/root
ENV DCORE_USER=root
USER $DCORE_USER
WORKDIR $DCORE_HOME

# DCore
ARG DCORE_VERSION
COPY dcore_$DCORE_VERSION-ubuntu${IMAGE_VERSION}_$TARGETARCH.deb /tmp
RUN mkdir $DCORE_HOME/.decent && dpkg -i /tmp/dcore_$DCORE_VERSION-ubuntu${IMAGE_VERSION}_$TARGETARCH.deb && rm /tmp/dcore_*.deb

EXPOSE 40000
EXPOSE 8090

ENV DCORE_EXTRA_ARGS=""
CMD decentd -d $DCORE_HOME/.decent/data --rpc-endpoint 0.0.0.0:8090 $DCORE_EXTRA_ARGS
