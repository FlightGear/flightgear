%define name flightgear
%define version 0.9.3
%define release 1grk

Summary: The FlightGear Flight Simulator
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Games/Other
BuildRoot: %{_tmppath}/%{name}-buildroot
Source: ftp://ftp.flightgear.org/pub/fgfs/Source/FlightGear-%{version}.tar.gz
Source1: ftp://ftp.flightgear.org/pub/fgfs/Shared/fgfs-base-%{version}.tar.gz
Source3: flightgear.desktop
Source10: %{name}.48.png
BuildRequires: plib >= 1.6.0, simgear = 0.3.4,  XFree86-devel, XFree86-Mesa-libGL, XFree86-Mesa-libGLU, gcc, zlib-devel
Requires: XFree86-devel, XFree86-Mesa-libGL, XFree86-Mesa-libGLU, gcc, zlib-devel
URL: http://www.flightgear.org
Obsoletes: FlightGear
Provides: FlightGear = %{version}-%{release}

%description
The Flight Gear project is working to create a sophisticated flight simulator
framework for the development and pursuit of interesting flight simulator
ideas. We are developing a solid basic sim that can be expanded and improved
upon by anyone interested in contributing.

%prep
%setup -q -n FlightGear-%{version}
rm -f docs-mini/*~

%build
%configure
make

%install
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT%{_libdir}
tar xzf %{SOURCE1} -C $RPM_BUILD_ROOT%{_libdir}
mv $RPM_BUILD_ROOT%{_libdir}/FlightGear-%{version} $RPM_BUILD_ROOT%{_libdir}/FlightGear

mkdir -p $RPM_BUILD_ROOT/%{_datadir}/applications/
cp %{SOURCE3} $RPM_BUILD_ROOT/%{_datadir}/applications/

desktop-file-install --vendor flightgear --delete-original \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  $RPM_BUILD_ROOT%{_datadir}/applications/flightgear.desktop

mkdir -p $RPM_BUILD_ROOT%{_datadir}/pixmaps
cp %{SOURCE10} $RPM_BUILD_ROOT%{_datadir}/pixmaps/%{name}.png

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
* Sun Oct 26 2003 Richard Keech <rkeech@redhat.com>
- updated for 0.9.3

* Sun Dec 29 2002 Richard Keech <rkeech@redhat.com>
- re-worked spec for Red Hat for 0.9.1.  
  Adapted from the one created
  by Lenny Cartier <lenny@mandrakesoft.com>, 
  Guillaume Cottenceau <gc@mandrakesoft.com> et al.

