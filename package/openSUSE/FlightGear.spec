Summary:    The FlightGear Flight Simulator
Name:       FlightGear
Version:    2.8.0
Release:    1
License:    GPL-2.0
URL:        http://www.flightgear.org
Group:      Amusements/Games/3D/Simulation
BuildRoot:  %{_tmppath}/%{name}-buildroot
Source0:    http://mirrors.ibiblio.org/pub/mirrors/flightgear/ftp/Source/flightgear-%{version}.tar.bz2
Source1:    %{name}.desktop

BuildRequires: gcc, gcc-c++, cmake
BuildRequires: update-desktop-files
BuildRequires: SimGear = %{version}, SimGear-devel = %{version}
BuildRequires: libOpenSceneGraph-devel >= 3.0
BuildRequires: subversion-devel, libapr1-devel
BuildRequires: freealut, freealut-devel
BuildRequires: libopenal1-soft, openal-soft
BuildRequires: zlib, zlib-devel
BuildRequires: libfreetype6
BuildRequires: libjpeg62, libjpeg62-devel
BuildRequires: libudev-devel
BuildRequires: boost-devel >= 1.37

Requires: OpenSceneGraph-plugins >= 3.0
Requires: FlightGear-data = %{version}
Requires: SimGear = %{version}

%description
The FlightGear project is working to create a sophisticated flight simulator
framework for the development and pursuit of interesting flight simulator
ideas. We are developing a solid basic sim that can be expanded and improved
upon by anyone interested in contributing.

%prep
%setup -q -n flightgear-%{version} -T -b 0
rm -f docs-mini/*
# remove unnecessary DATE/TIME dependency
sed -i 's/__DATE__" "__TIME__//' src/FDM/JSBSim/FGJSBBase.cpp

%build
export CFLAGS="$RPM_OPT_FLAGS"
export CXXFLAGS="$RPM_OPT_FLAGS"
export BUILD_ID=OpenSuSE
export BUILD_NUMBER=0
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DSIMGEAR_SHARED:BOOL=ON -DENABLE_TESTS:BOOL=OFF -DFG_DATA_DIR:STRING="/usr/share/flightgear" -DJPEG_FACTORY:BOOL=ON
make %{?_smp_mflags}

%install
make %{?_smp_mflags} install DESTDIR=$RPM_BUILD_ROOT
# install desktop link
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/applications/
cp %{SOURCE1} $RPM_BUILD_ROOT/%{_datadir}/applications/
%suse_update_desktop_file -i %{name}
# move docs into doc/packages subfolder
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/packages/FlightGear
cp README $RPM_BUILD_ROOT/usr/share/doc/packages/FlightGear/.
cp COPYING $RPM_BUILD_ROOT/usr/share/doc/packages/FlightGear/.
cp AUTHORS $RPM_BUILD_ROOT/usr/share/doc/packages/FlightGear/.
# install icon
mkdir -p $RPM_BUILD_ROOT%{_datadir}/pixmaps
cp icons/fg-128.png $RPM_BUILD_ROOT%{_datadir}/pixmaps/FlightGear.png
# remove obsolete/test utilities
cd $RPM_BUILD_ROOT/usr/bin && rm -f GPSsmooth MIDGsmooth UGsmooth metar

%files
%defattr(-, root, root, -)
%dir /usr/share/doc/packages/FlightGear
/usr/share/doc/packages/FlightGear/*
%{_bindir}/*
%_mandir/man1/*
%{_datadir}/pixmaps/FlightGear.png
%{_datadir}/applications/%{name}.desktop

%changelog
* Thu Jun 30 2012 thorstenb@flightgear.org
- Initial version

