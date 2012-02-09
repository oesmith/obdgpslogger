#
# spec file for package: obdgpslogger
#
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# include module(s): obdgpslogger
#

%include Solaris.inc

Name:           obdgpslogger
Summary:        Logging and analysis tools for OBDII and GPS data
Version:        0.16
License:        GPLv2
Source:         http://icculus.org/obdgpslogger/downloads/%{name}-%{version}.tar.gz
URL:            http://icculus.org/obdgpslogger/
Group:          Utilities
Distribution:	OpenSolaris
Vendor:		OpenSolaris Community

# OpenSolaris IPS Manifest Fields
Meta(info.upstream): Gary Briggs <chunky@icculus.org>
Meta(info.repository_url): svn://svn.icculus.org/obdgpslogger/
Meta(info.maintainer): Gary Briggs <chunky@icculus.org>
Meta(info.classification): org.opensolaris.category.2008:Applications/Accessories

%include default-depend.inc
Requires: SUNWdbus
Requires: SUNWdbus-libs
Requires: SUNWzlib
BuildRequires: SUNWcmake
BuildRequires: SUNWgcc
BuildRequires: SUNWgmake
# Someday, when gpsd and fltk make their way into opensolaris...
# Requires: gpsd
# Requires: libfltk1.1
# BuildRequires: libfltk1.1-dev
# BuildRequires: fluid
# BuildRequires: libgps-dev

BuildRoot:      %{_tmppath}/%{name}-%{version}-build
SUNW_BaseDir:   %{_basedir}
SUNW_Copyright: %{name}.copyright

%description
obdgpslogger is a tool to log OBDII and GPS data from your car while driving. It includes tools to export that data to KML and CSV, and an OBDII simulator.

%prep
rm -rf %{name}-%{version}
%setup -q -n %{name}-%{version}

%build
mkdir build
cd build
echo "Prefix: %{_prefix}"
echo "RPM_BUILD_ROOT: ${RPM_BUILD_ROOT}"
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DOBD_DISABLE_GUI=true ..
make

%install
cd build
# If/when cmake doesn't get hardcoded to install to build/sfw_stage:
# DESTDIR=$RPM_BUILD_ROOT make install
make install

ls -lR sfw_stage
cd sfw_stage
mkdir -p $RPM_BUILD_ROOT/%{_prefix}
cp -r -pr * $RPM_BUILD_ROOT/%{_prefix}

%clean
rm -rf build $RPM_BUILD_ROOT

%files
%defattr (-, root, bin)
%dir %attr (0755, root, bin) %{_bindir}
%{_bindir}/*
%dir %attr(0755, root, sys) /usr/share
%dir %attr(0755, root, bin) %{_mandir}
%dir %attr(0755, root, bin) %{_mandir}/man{1,5}
%{_mandir}/man1/*.1
%{_mandir}/man5/*.5


%changelog
* Thu May 26 2011 - Gary Briggs <chunky@icculus.org>
- New upstream version
* Sat Feb 27 2010 - Gary Briggs <chunky@icculus.org>
- New upstream version
* Tue Jan 12 2010 - Gary Briggs <chunky@icculus.org>
- Change license, add info.classification field
* Sun Jan 3 2010 - Gary Briggs <chunky@icculus.org>
- initial version


