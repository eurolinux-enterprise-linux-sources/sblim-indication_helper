sblimincdir		= $(includedir)/sblim

INCLUDES 		= -I$(top_srcdir) 

RPMREL                  = @RPMREL@
RPM                     = @RPM@
RPMFLAGS                = -ta
TARFILE                 = $(PACKAGE)-$(VERSION).tar.bz2

DESTDIR                 =
RPMDIR                  =$(HOME)/rpms
RPMSRC  =$(DESTDIR)$(RPMDIR)/SRPMS/$(PACKAGE)-$(VERSION)-$(RPMREL).src.rpm
RPM386  =$(DESTDIR)$(RPMDIR)/RPMS/i386/$(PACKAGE)-$(VERSION)-$(RPMREL).i386.rpm


lib_LTLIBRARIES		= libind_helper.la

SUBDIRS                 = test 
libind_helper_la_SOURCES	= ind_helper.c
libind_helper_la_LDFLAGS	= @LIBPTHREAD@
sbliminc_HEADERS	= $(top_srcdir)/ind_helper.h

EXTRA_DIST		=  sblim-indication_helper.spec

rpm: dist-bzip2
	$(RPM) $(RPMFLAGS) $(TARFILE);


