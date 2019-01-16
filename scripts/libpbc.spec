Name: libpbc
URL: https://crypto.stanford.edu/pbc
Version: 0.5.14
Release: 1%{?dist}
License: Public Domain
Summary: Pairing-Based Crypto library
Source0: https://crypto.stanford.edu/pbc/files/pbc-%{version}.tar.gz

%description
Pairing-based cryptography is a relatively young area of cryptography that
revolves around a certain function with special properties. The PBC library is
designed to be the backbone of implementations of pairing-based cryptosystems,
thus speed and portability are important goals. It provides routines such as
elliptic curve generation, elliptic curve arithmetic and pairing computation.

%package devel
Summary: Development files for %{name} library
Requires: %{name} = %{version}-%{release}

%description devel
This package contains the header files for developing programs which
will use the %{name} library.

%package static
Summary: Static %{name} library
Requires: %{name}-devel = %{version}-%{release}

%description static
This package contains statically linkable version %{name} library.

%prep
wget https://crypto.stanford.edu/pbc/files/pbc-%{version}.tar.gz
rm -rf pbc-%{version}
tar xvf pbc-%{version}.tar.gz
rm pbc-%{version}.tar.gz

%build
cd pbc-%{version}
./setup && ./configure --prefix=%{buildroot}/usr --libdir=%{buildroot}%{_libdir} && make

%install
cd pbc-%{version}
make install
rm %{buildroot}%{_libdir}/*.la

%clean
rm -rf %{buildroot}

%files
%{_libdir}/libpbc.so.*

%files devel
%{_includedir}/*
%{_libdir}/libpbc.so

%files static
%{_libdir}/libpbc.a

%changelog
