Name: DCore
URL: https://decent.ch
Version: %{dcore_version}
Release: 1%{?dist}
License: GPLv3
Summary: Fast, powerful and cost-efficient blockchain
Source0: https://github.com/DECENTfoundation/DECENT-Network/archive/%{version}.tar.gz
Requires: libpbc = %{pbc_version}, openssl-libs >= 1.1, cryptopp >= 6.1, readline >= 7.0, ncurses-libs >= 6.1, libcurl, gmp, zlib

%{?systemd_requires}
BuildRequires: systemd, boost-devel >= 1.65.1

%description
DCore is the blockchain you can easily build on. As the world’s first blockchain
designed for digital content, media and entertainment, it provides user-friendly
software development kits (SDKs) that empower developers and businesses to build
decentralized applications for real-world use cases. DCore packed-full of
customizable features making it the ideal blockchain for any size project.

%package GUI
Summary: Fast, powerful and cost-efficient blockchain - GUI client
Requires: libpbc = %{pbc_version}, qt5-qtbase >= 5.11, openssl-libs >= 1.1, cryptopp >= 6.1, readline >= 7.0, ncurses-libs >= 6.1, libcurl, gmp, zlib
BuildRequires: qt5-qtbase-devel >= 5.11, qt5-linguist >= 5.11

%description GUI
DCore is the blockchain you can easily build on. As the world’s first blockchain
designed for digital content, media and entertainment, it provides user-friendly
software development kits (SDKs) that empower developers and businesses to build
decentralized applications for real-world use cases. DCore packed-full of
customizable features making it the ideal blockchain for any size project.

%prep
git clone --single-branch --branch %{git_revision} https://github.com/DECENTfoundation/DECENT-Network.git
cd DECENT-Network
git submodule update --init --recursive

%build
mkdir DECENT-Network/build
cd DECENT-Network/build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{_builddir}/DCore ..
make -j$(nproc) install

%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_unitdir}
cp %{_builddir}/DECENT-Network/%{name}.service %{buildroot}%{_unitdir}
for f in %{_builddir}/DCore/bin/*; do
    strip $f -o %{buildroot}%{_bindir}/$(basename $f) && chrpath -d %{buildroot}%{_bindir}/$(basename $f)
done

%clean
rm -rf DECENT-Network
rm -rf %{buildroot}

%files
%{_bindir}/cli_wallet
%{_bindir}/decentd
%{_unitdir}/DCore.service

%files GUI
%{_bindir}/DECENT

%post
%systemd_post %{name}.service

%preun
%systemd_preun %{name}.service

%changelog
