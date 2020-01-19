Name:		sblim-indication_helper
Version:	0.4.2
Release:	12%{?dist}
Summary:	Toolkit for CMPI indication providers

Group:		Development/Libraries
License:	CPL
URL:		http://sblim.wiki.sourceforge.net/
Source0:	http://downloads.sourceforge.net/sblim/%{name}-%{version}.tar.bz2
Patch0:		sblim-indication_helper-0.4.2_warnings.patch
Patch1:		missing-stderr-def.patch
BuildRequires:	sblim-cmpi-devel 

%description
This package contains a developer library for helping out when writing
CMPI providers. This library polls the registered functions for data
and, if it changes, a CMPI indication is set with the values of the
indication class properties (also set by the developer).

%Package	devel
Summary:	Toolkit for CMPI indication providers (Development Files)
Requires:	%{name} = %{version}-%{release} sblim-cmpi-devel glibc-devel
Group:		Development/Libraries


%description devel
This package contain developer library for helping out when writing
CMPI providers. This library polls the registered functions for data
and if they change an CMPI indication is set with the values of the
indication class properties (also set by the developer).

This package holds the development files for sblim-indication_helper.


%prep
%setup -q
%patch0
%patch1 -p1


%build
%configure --disable-static --with-pic
make %{?_smp_mflags}


%install
make install DESTDIR=$RPM_BUILD_ROOT
rm $RPM_BUILD_ROOT/%{_libdir}/libind_helper.la

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%doc README COPYING ChangeLog TODO
%{_libdir}/libind_helper.so.*



%files devel
%{_includedir}/sblim
%{_libdir}/libind_helper.so
%doc COPYING


%changelog
* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 0.4.2-12
- Mass rebuild 2014-01-24

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 0.4.2-11
- Mass rebuild 2013-12-27

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.4.2-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Mon Nov 19 2012 Vitezslav Crhonek <vcrhonek@redhat.com> - 0.4.2-9
- Fix mixed use of spaces and tabs in specfile

* Thu Sep 06 2012 Vitezslav Crhonek <vcrhonek@redhat.com> - 0.4.2-8
- Fix issues found by fedora-review utility in the spec file

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.4.2-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.4.2-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.4.2-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Mon Dec  7 2009 Praveen K Paladugu <praveen_paladugu@dell.com> - 0.4.2-4
-  fixed the build failure because of missing definition of stderr
* Fri Jul 31 2009 Praveen K Paladugu <praveen_paladugu@dell.com> - 0.4.2-3
- fixed the rpmlint message. Removed Requries for glibc-devel.
* Tue Jun 30 2009 Praveen K Paladugu <praveen_paladugu@dell.com> - 0.4.2-1
- Standardized the spec file and changed the build number to 1 
* Thu Oct 23 2008 Matt Domsch <Matt_Domsch@dell.com> - 0.4.2-134
- update for Fedora packaging guidelines
* Fri May 30 2008 npaxton@novell.com
- Change openwbem-devel dependency to sblim-cmpi-devel, to be
  cimom neutral
* Wed Feb 27 2008 crrodriguez@suse.de
- fix library-without-ldconfig* errors
- disable static libraries
* Wed Mar 01 2006 mrueckert@suse.de
- update to 0.4.2
  ind_helper.c, ind_helper.h:
  Bugs: 1203849 (side effect) made a lot of function arguments
  const in order to remove the cmpi-base warnings.
  added sblim-indication_helper-0.4.2_warnings.patch
  fixes a small warning regarding pointer size
* Wed Jan 25 2006 mls@suse.de
- created the package

