%define name FlightGear
%define version 0.7.8
%define release 2mdk
%define prefix %{_prefix}

Summary: The FlightGear Flight Simulator
Name: %{name}
Version: %{version}
Release: %{release}
Copyright: GPL
Group: Games/Other
BuildRoot: %{_tmppath}/%{name}-buildroot
Source: http://prdownloads.sourceforge.net/flightgear/%{name}-%{version}.tar.bz2
Source1: ftp://flightgear.sourceforge.net:/pub/flightgear/Shared/fgfs_base.%{version}.tar.bz2
Source10: %{name}.16.xpm.bz2
Source11: %{name}.32.xpm.bz2
Source12: %{name}.48.xpm.bz2
Patch1: FlightGear-0.7.8-fix-sound.patch.bz2
Packager: Guillaume Cottenceau <gc@mandrakesoft.com>
BuildRequires: plib >= 1.4.2 SimGear SimGear-devel >= 0.0.16
Requires: SimGear >= 0.0.16
URL: http://www.flightgear.org

%description
The Flight Gear project is working to create a sophisticated flight simulator
framework for the development and pursuit of interesting flight simulator
ideas. We are developing a solid basic sim that can be expanded and improved
upon by anyone interested in contributing.

%prep
%setup -q
%patch1 -p0

%build
%configure --bindir=%{_gamesbindir} --without-logging
make

%install
%makeinstall bindir=$RPM_BUILD_ROOT/%{_gamesbindir}
mkdir -p $RPM_BUILD_ROOT%{_libdir}
tar xjvf %{SOURCE1} -C $RPM_BUILD_ROOT%{_libdir}

mkdir -p $RPM_BUILD_ROOT/%{_menudir}
cat << EOF > $RPM_BUILD_ROOT/%{_menudir}/%{name}
?package(%{name}):command="%{_gamesbindir}/fgfs" icon="%{name}.xpm" \
  needs="x11" section="Amusement/Other" title="%{name}" \
  longtitle="The FlightGear Flight Simulator"
EOF

mkdir -p $RPM_BUILD_ROOT%{_miconsdir}
mkdir -p $RPM_BUILD_ROOT%{_liconsdir}
bzcat %{SOURCE10} > $RPM_BUILD_ROOT%{_miconsdir}/%{name}.xpm
bzcat %{SOURCE11} > $RPM_BUILD_ROOT%{_iconsdir}/%{name}.xpm
bzcat %{SOURCE12} > $RPM_BUILD_ROOT%{_liconsdir}/%{name}.xpm

%post
%{update_menus}

%postun
%{clean_menus}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc README COPYING AUTHORS docs-mini/
%{_gamesbindir}/*
%{_libdir}/%{name}
%{_menudir}/%{name}
%{_miconsdir}/%{name}.xpm
%{_iconsdir}/%{name}.xpm
%{_liconsdir}/%{name}.xpm

%changelog
* Thu Sep 13 2001 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.8-2mdk
- patch init of sound

* Mon Jul 23 2001 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.8-1mdk
- version 0.7.8

* Thu Jun 21 2001 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.7-1mdk
- version 0.7.7

* Fri Jan  5 2001 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.6-2mdk
- rebuild

* Wed Dec 20 2000 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.6-1mdk
- 0.7.6

* Tue Sep 26 2000 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.5-1mdk
- rebuild to remove dependency to libglide.so, thanks to Francois
- added menu entry
- shiny new 0.7.5

* Mon Jul 24 2000 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.4-2mdk
- 0.7.4 final

* Tue Jul  4 2000 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.4-1mdk
- 0.7.4
- ships with the data now

* Thu Jun 29 2000 Guillaume Cottenceau <gc@mandrakesoft.com> 0.7.3-1mdk
- first Mandrake Package

