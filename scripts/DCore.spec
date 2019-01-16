Name: DCore
URL: https://decent.ch
Version: %{dcore_version}
Release: 1%{?dist}
License: Public Domain
Summary: Fast, powerful and cost-efficient blockchain
Source0: https://github.com/DECENTfoundation/DECENT-Network/archive/%{version}.tar.gz
Requires: libpbc = 0.5.14

%{?systemd_requires}
BuildRequires: systemd, boost-devel >= 1.65.1, qt5-qtbase-devel >= 5.11

%description
DCore is the blockchain you can easily build on. As the world’s first blockchain
designed for digital content, media and entertainment, it provides user-friendly
software development kits (SDKs) that empower developers and businesses to build
decentralized applications for real-world use cases. DCore packed-full of
customizable features making it the ideal blockchain for any size project.

%package GUI
Requires: libpbc = 0.5.14, qt5-qtbase >= 5.11
Summary: Fast, powerful and cost-efficient blockchain - GUI client

%description GUI
DCore is the blockchain you can easily build on. As the world’s first blockchain
designed for digital content, media and entertainment, it provides user-friendly
software development kits (SDKs) that empower developers and businesses to build
decentralized applications for real-world use cases. DCore packed-full of
customizable features making it the ideal blockchain for any size project.

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
mkdir -p %{buildroot}%{_unitdir}
cp %{_builddir}/DECENT-Network/%{name}.service %{buildroot}%{_unitdir}
for f in %{_builddir}/DECENT-Network/build/artifacts/prefix/bin/*; do
    strip $f -o %{buildroot}%{_bindir}/$(basename $f) && chrpath -d %{buildroot}%{_bindir}/$(basename $f)
done

%clean
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
