
# (C) Copyright IBM Corp. 2004, 2009
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Eclipse Public License from
#* http://www.opensource.org/licenses/eclipse-1.0.php
#
# Author:       Konrad Rzeszutek <konradr@us.ibm.com>
# Date  :	      09/20/2004
#

Name: sblim-indication_helper
Summary: sblim-indication_helper - Indication helper library for CMPI providers
Version: 0.5.0
Release: 1
Copyright: Eclipse Public Licence 1.0 
Group: System Management/Libraries
Vendor: SBLIM 
Packager: Konrad Rzeszutek <konradr@us.ibm.com>
Source: sblim-indication_helper-0.5.0.tar.bz2
Buildroot: /var/tmp/sblim-indication_helper-root


%description 
This package contain developer library for helping out when writing CMPI providers. This library
polls the registered functions for data and if they change an CMPI indication is set with the
values of the indication class properties (also set by the developer). 

###################################################
%prep
###################################################

###################################################
%setup
###################################################

###################################################
%build
###################################################
%configure

make

###################################################
%install
###################################################
if
  [ ! -z "${RPM_BUILD_ROOT}"  -a "${RPM_BUILD_ROOT}" != "/" ]
then
  rm -rf $RPM_BUILD_ROOT
fi
make DESTDIR=$RPM_BUILD_ROOT install

%post

###################################################
%files
###################################################
%defattr(-,root,root)
%doc README COPYING ChangeLog TODO
%{_includedir}
%{_libdir}/lib*

###################################################
%clean
###################################################
if
    [ -z "${RPM_BUILD_ROOT}"  -a "${RPM_BUILD_ROOT}" != "/" ]
then
    rm -rf $RPM_BUILD_ROOT
fi
rm -rf $RPM_BUILD_DIR/sblim-indication_helper-0.5.0

