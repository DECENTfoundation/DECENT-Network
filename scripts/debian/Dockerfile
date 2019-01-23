ARG IMAGE_VERSION=latest
FROM debian:$IMAGE_VERSION
LABEL maintainer="DECENT"

# prerequisites
RUN . /etc/os-release && apt-get update && \
    apt-get install -y --no-install-recommends \
        libreadline7 \
        libcrypto++6 \
        libssl1.1 \
        libcurl3 \
        wget \
        ca-certificates \
    && apt-get clean && apt-get autoremove

# PBC
ARG PBC_VERSION=0.5.14
RUN . /etc/os-release && \
    wget -nv -P /tmp https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc_$PBC_VERSION-debian${VERSION_ID}_amd64.deb && \
    dpkg -i /tmp/libpbc_$PBC_VERSION-debian${VERSION_ID}_amd64.deb && \
    rm /tmp/libpbc_$PBC_VERSION-debian${VERSION_ID}_amd64.deb

# prepare directories
ENV DCORE_HOME=/root
ENV DCORE_USER=root
USER $DCORE_USER
WORKDIR $DCORE_HOME

# DCore
ARG DCORE_VERSION
COPY dcore_$DCORE_VERSION-*.deb /tmp
RUN mkdir $DCORE_HOME/.decent && dpkg -i /tmp/dcore_$DCORE_VERSION-* && rm /tmp/dcore_$DCORE_VERSION-*
EXPOSE 40000 8090 5001

ENV DCORE_EXTRA_ARGS=""
CMD decentd -d $DCORE_HOME/.decent/data --rpc-endpoint 0.0.0.0:8090 $DCORE_EXTRA_ARGS
