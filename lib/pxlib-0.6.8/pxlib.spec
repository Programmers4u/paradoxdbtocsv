Summary: A library to read Paradox DB files
Name: pxlib
Version: 0.6.8
Release: 1
License: see doc/COPYING
Group: Applications/Utils
URL: http://%{name}.sourceforge.net/
Packager: Uwe Steinmann <uwe@steinmann.cx>
Source: http://prdownloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
%{name} is a simply and still small C library to read Paradox DB files. It
supports all versions starting from 3.0. It currently has a very limited set of
functions like to open a DB file, read its header and read every single record.

%package devel
Summary: A library to read Paradox DB files (Development)
Group: Development/Libraries
Requires: %{name} = %{version}

%description devel
%{name}pxlib is a simply and still small C library to read Paradox DB files. It
supports all versions starting from 3.0. It currently has a very limited set of
functions like to open a DB file, read its header and read every single record.

%prep
%setup -q

%build
%configure --with-sqlite --with-gsf
make

%install
rm -rf ${RPM_BUILD_ROOT}
install -d -m 755 ${RPM_BUILD_ROOT}
make DESTDIR=${RPM_BUILD_ROOT} install

%clean
rm -rf ${RPM_BUILD_ROOT}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%attr(-,root,root) %doc README AUTHORS ChangeLog COPYING INSTALL
%attr(-,root,root) %{_libdir}/lib*.so.*
%attr(-,root,root) %{_datadir}/locale/*/LC_MESSAGES/*

%files devel
%attr(-,root,root) %{_libdir}/lib*.so
%attr(-,root,root) %{_libdir}/*.a
%attr(-,root,root) %{_libdir}/*.la
%attr(-,root,root) %{_libdir}/pkgconfig/*
%attr(-,root,root) %{_includedir}/*

%changelog
* Sun May 15 2011 Mihai T. Lazarescu <mihai@lazarescu.org> - 0.6.3-1
- use macros in preamble and description
- use system default prefix
- fixed package %version reference
- set BuildRoot to system-defined directory
- quiet archive expansion in %setup
- use system %configure macro for default system setup
- removed man pages from the %files section
