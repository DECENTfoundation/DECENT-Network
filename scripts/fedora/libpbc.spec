Name: libpbc
URL: https://crypto.stanford.edu/pbc
Version: %{pbc_version}
Release: 1%{?dist}
License: LGPLv3
Summary: Pairing-Based Crypto library
Source0: https://github.com/DECENTfoundation/pbc/archive/%{version}.tar.gz

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
This package contains statically linkable %{name} library.

%prep
rm -rf pbc
git clone --single-branch --branch %{git_revision} https://github.com/DECENTfoundation/pbc.git

%build
cd pbc
./setup && ./configure --prefix=%{buildroot}/usr --libdir=%{buildroot}%{_libdir} && make

%install
cd pbc
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
