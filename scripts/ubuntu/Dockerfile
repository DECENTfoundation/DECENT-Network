ARG IMAGE_VERSION=latest
FROM ubuntu:$IMAGE_VERSION
LABEL maintainer="DECENT"

# prerequisites
RUN . /etc/os-release && apt-get update && \
    if [ "$VERSION_ID" != "16.04" ]; then apt-get install -y --no-install-recommends \
        libreadline7 \
        libcrypto++6 \
        libssl1.1 \
        libcurl4 \
        less \
        wget \
        ca-certificates; \
    else apt-get install -y --no-install-recommends \
        libreadline6 \
        libcrypto++9v5 \
        libssl1.0.0 \
        libcurl3 \
        less \
        wget \
        ca-certificates; \
    fi && apt-get clean && apt-get autoremove

# PBC
ARG PBC_VERSION=0.5.14
RUN . /etc/os-release && export ARCH=`dpkg --print-architecture` && \
    wget -nv -P /tmp https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc_$PBC_VERSION-${ID}${VERSION_ID}_${ARCH}.deb && \
    dpkg -i /tmp/libpbc_$PBC_VERSION-${ID}${VERSION_ID}_${ARCH}.deb && \
    rm /tmp/libpbc_$PBC_VERSION-${ID}${VERSION_ID}_${ARCH}.deb

# DCore
ARG DCORE_VERSION
COPY dcore_${DCORE_VERSION}-*.deb /tmp/
RUN . /etc/os-release && export ARCH=`dpkg --print-architecture` && \
    dpkg -i /tmp/dcore_$DCORE_VERSION-${ID}${VERSION_ID}_${ARCH}.deb && \
    rm /tmp/dcore_${DCORE_VERSION}-*.deb

EXPOSE 40000 8090

ENV DCORE_USER=dcore
ENV DCORE_HOME=/home/dcore
RUN useradd --home-dir $DCORE_HOME --create-home $DCORE_USER && mkdir $DCORE_HOME/.decent
USER $DCORE_USER
WORKDIR $DCORE_HOME

ENV DCORE_EXTRA_ARGS=""
ENV DCORE_DATA_DIR=$DCORE_HOME"/.decent/data"
CMD decentd -d $DCORE_DATA_DIR --rpc-endpoint 0.0.0.0:8090 $DCORE_EXTRA_ARGS
