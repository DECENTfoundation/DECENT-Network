ARG IMAGE_VERSION=latest
FROM fedora:$IMAGE_VERSION
LABEL maintainer="DECENT"

# prerequisites
RUN dnf install -y \
    automake \
    autoconf \
    libtool \
    make \
    cmake \
    gcc-c++ \
    chrpath \
    flex \
    bison \
    doxygen \
    unzip \
    which \
    wget \
    git \
    qt5-qtbase-devel \
    qt5-linguist \
    rpm-build \
    rpm-devel \
    rpmlint \
    rpmdevtools \
    readline-devel \
    cryptopp-devel \
    gmp-devel \
    openssl-devel \
    libcurl-devel \
    boost-devel \
    boost-static && \
    dnf clean all

# prepare directories
USER root
WORKDIR /root
RUN rpmdev-setuptree && export ARCH=`rpm -E "%{_arch}"` && mkdir rpmbuild/RPMS/$ARCH && ln -s rpmbuild/RPMS/$ARCH /root/packages

# PBC
ARG PBC_VERSION=0.5.14
ARG PBC_GIT_REV=0.5.14
COPY libpbc.spec rpmbuild/SPECS
RUN export ARCH=`rpm -E "%{_arch}"` && \
    rpmbuild -bb -D "pbc_version $PBC_VERSION" -D "git_revision $PBC_GIT_REV" --rmspec rpmbuild/SPECS/libpbc.spec && \
    rpm -i rpmbuild/RPMS/$ARCH/libpbc* && \
    rm rpmbuild/RPMS/$ARCH/libpbc*

CMD ["/bin/bash"]
