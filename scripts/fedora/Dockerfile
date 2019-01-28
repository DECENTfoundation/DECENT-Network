ARG IMAGE_VERSION=latest
FROM fedora:$IMAGE_VERSION
LABEL maintainer="DECENT"

# prerequisites
RUN dnf install -y cryptopp wget procps && dnf clean all

# PBC
ARG PBC_VERSION=0.5.14
RUN export FEDORA=`rpm -E "%{fedora}"` && export ARCH=`rpm -E "%{_arch}"` && \
    wget -nv -P /tmp https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc-$PBC_VERSION-1.fc$FEDORA.$ARCH.rpm && \
    rpm -i /tmp/libpbc-$PBC_VERSION-* && rm /tmp/libpbc-$PBC_VERSION-*

# prepare directories
ENV DCORE_HOME=/root
ENV DCORE_USER=root
USER $DCORE_USER
WORKDIR $DCORE_HOME

# DCore
ARG DCORE_VERSION
COPY DCore-$DCORE_VERSION-1.fc*.rpm /tmp
RUN export FEDORA=`rpm -E "%{fedora}"` && \
    export ARCH=`rpm -E "%{_arch}"` && \
    rpm -i /tmp/DCore-$DCORE_VERSION-1.fc$FEDORA.$ARCH.rpm && \
    rm /tmp/DCore-$DCORE_VERSION-1.fc*.rpm && \
    mkdir $DCORE_HOME/.decent

EXPOSE 40000 8090 5001

ENV DCORE_EXTRA_ARGS=""
ENV DCORE_DATA_DIR=$DCORE_HOME"/.decent/data"
CMD decentd -d $DCORE_DATA_DIR --rpc-endpoint 0.0.0.0:8090 $DCORE_EXTRA_ARGS
