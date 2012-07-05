%define name flightgear
%define version 2.8.0
%define release 1

Summary: The FlightGear Flight Simulator
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL-2.0
URL: http://www.flightgear.org
Group: Games/Other
BuildRoot: %{_tmppath}/%{name}-buildroot
Source: http://mirrors.ibiblio.org/pub/mirrors/flightgear/ftp/Source/flightgear-%{version}.tar.bz2
Source1: http://mirrors.ibiblio.org/pub/mirrors/flightgear/ftp/Shared/FlightGear-data-%{version}.tar.bz2
Source3: flightgear.desktop
BuildRequires: gcc, gcc-c++, cmake
BuildRequires: plib >= 1.8.0, SimGear = %{version}
BuildRequires: XFree86-devel, XFree86-Mesa-libGL, XFree86-Mesa-libGLU, zlib-devel
BuildRequires: OpenSceneGraph >= 3.0.0
Obsoletes: FlightGear
Provides: FlightGear = %{version}-%{release}

%description
The FlightGear project is working to create a sophisticated flight simulator
framework for the development and pursuit of interesting flight simulator
ideas. We are developing a solid basic sim that can be expanded and improved
upon by anyone interested in contributing.

%prep
%setup -q -n %{name}-%{version}

%build
cmake -DSIMGEAR_SHARED:BOOL=ON -DENABLE_TESTS:BOOL=OFF -DFG_DATA_DIR:STRING="/usr/share/flightgear" -DJPEG_FACTORY:BOOL=ON -DCMAKE_INSTALL_PREFIX:PATH="/usr"
make

%install
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_libdir}
tar xzf %{SOURCE1} -C $RPM_BUILD_ROOT%{_libdir}
mv $RPM_BUILD_ROOT%{_libdir}/FlightGear-%{version} $RPM_BUILD_ROOT%{_libdir}/FlightGear

mkdir -p $RPM_BUILD_ROOT/%{_datadir}/applications/
cp %{SOURCE3} $RPM_BUILD_ROOT/%{_datadir}/applications/

desktop-file-install --vendor flightgear --delete-original \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  $RPM_BUILD_ROOT%{_datadir}/applications/flightgear.desktop

# install icon
mkdir -p $RPM_BUILD_ROOT%{_datadir}/pixmaps
cp icons/fg-128.png $RPM_BUILD_ROOT%{_datadir}/pixmaps/FlightGear.png

%post

%postun

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc README COPYING AUTHORS docs-mini/
%{_bindir}/*
%{_libdir}/FlightGear
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/applications/flightgear.desktop
%_mandir/man1/*

%changelog
* Thu Jun 28 2012 Thorsten Brehm <thorstenb@flightgear.org>
- Updated to 2.8.0
- Converted to CMake
- Use shared SimGear libraries

* Sun Oct 26 2003 Richard Keech <rkeech@redhat.com>
- updated for 0.9.3

* Sun Dec 29 2002 Richard Keech <rkeech@redhat.com>
- re-worked spec for Red Hat for 0.9.1.  
  Adapted from the one created
  by Lenny Cartier <lenny@mandrakesoft.com>, 
  Guillaume Cottenceau <gc@mandrakesoft.com> et al.

