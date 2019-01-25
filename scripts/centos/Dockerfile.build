ARG IMAGE_VERSION=latest
FROM centos:$IMAGE_VERSION
LABEL maintainer="DECENT"

# prerequisites
RUN yum install -y wget && \
    wget -nv -P /tmp http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm && \
    rpm -ivh /tmp/epel-release-latest-7.noarch.rpm && \
    rm /tmp/epel-release-latest-7.noarch.rpm && \
    yum install -y \
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
    yum clean all

# prepare directories
USER root
RUN rpmdev-setuptree

# build PBC
ARG PBC_VERSION=0.5.14
ARG PBC_GIT_REV=0.5.14
COPY libpbc.spec /root/rpmbuild/SPECS
RUN export ARCH=`rpm -E "%{_arch}"` && \
    rpmbuild -bb -D "pbc_version $PBC_VERSION" -D "git_revision $PBC_GIT_REV" --rmspec /root/rpmbuild/SPECS/libpbc.spec && \
    rpm -i /root/rpmbuild/RPMS/$ARCH/libpbc* && rm -rf /root/rpmbuild/RPMS/$ARCH

CMD ["/bin/bash"]
