Name: DCore
URL: https://decent.ch
Version: %{dcore_version}
Release: 1%{?dist}
License: Public Domain
Summary: Fast, powerful, cost-efficient blockchain designed for digital content, media and entertainment distribution
Source0: https://github.com/DECENTfoundation/DECENT-Network/archive/%{version}.tar.gz

BuildRequires: boost-devel >= 1.65.1, qt5-qtbase-devel >= 5.11

%description
DCore is the blockchain you can easily build on. As the world’s first blockchain designed for digital content, media and entertainment, DCore provides user-friendly software development kits (SDKs) that empower developers and businesses to build decentralized applications for real-world use cases. DCore is fast, powerful, cost-efficient and packed-full of customizable features making it the ideal blockchain for any size project.

%package GUI
Requires: qt5-qtbase >= 5.11
Summary: Fast, powerful, cost-efficient blockchain designed for digital content, media and entertainment distribution

%description GUI
GUI client and node. DCore is the blockchain you can easily build on. As the world’s first blockchain designed for digital content, media and entertainment, DCore provides user-friendly software development kits (SDKs) that empower developers and businesses to build decentralized applications for real-world use cases. DCore is fast, powerful, cost-efficient and packed-full of customizable features making it the ideal blockchain for any size project.

%prep
rm -rf DECENT-Network
git clone --single-branch --branch %{git_revision} https://github.com/DECENTfoundation/DECENT-Network.git
cd DECENT-Network
git submodule update --init --recursive

%build
mkdir DECENT-Network/build
cd DECENT-Network/build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) install

%install
mkdir -p %{buildroot}%{_bindir}
cp %{_builddir}/DECENT-Network/build/artifacts/prefix/bin/* %{buildroot}%{_bindir}

%clean
rm -rf %{buildroot}

%files
%{_bindir}/cli_wallet
%{_bindir}/decentd

%files GUI
%{_bindir}/DECENT
