#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(SBLIM Indication Helper, 0.4.2, sblim-devel@lists.sourceforge.net,sblim-indication_helper)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([ind_helper.c])
AM_CONFIG_HEADER(config.h)

IND_HLP_PKG=sblim-indication_helper
AC_SUBST(IND_HLP_PKG)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_PATH_PROGS(RPM, rpmbuild)

dnl Check for -lpthread
AC_CHECK_LIB(pthread,pthread_create,AC_SUBST(LIBPTHREAD,"-lpthread"))

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_TYPE_SIGNAL

AC_CHECK_FUNCS([memset strdup strndup])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

CHECK_PEGASUS_2_3_2
CHECK_PEGASUS_2_4
CHECK_CMPI

AC_ARG_ENABLE([testcover],
    [  --enable-testcover     build with test coverage support [default=no]],
    [if test "x$enableval" = "xyes"; then
            CFLAGS=`echo $CFLAGS | perl -p -e 's/-O\S*//g'`
            CFLAGS="$CFLAGS -ftest-coverage -fprofile-arcs"
    fi]
    )

AC_ARG_ENABLE([debug],
    [  --enable-debug        turn on debug messages [default=no]],
    [if test "x$enableval" = "xyes"; then
            CFLAGS="$CFLAGS -DDEBUG"
    fi]
    )

# Now we throw in a bunch of warnings.  C and C++ need
# different sets here

CC_WARNINGS="\
	-Wall \
        -Wmissing-prototypes \
	-Wmissing-declarations \
	-Wstrict-prototypes \
	-Wpointer-arith \
        -Wformat=2 \
        -Wformat-security \
        -Wformat-nonliteral \
	-Wcast-qual \
	-Wcast-align"

CXX_WARNINGS="\
	-Wall\
        -Wmissing-prototypes \
	-Wstrict-prototypes \
	-Wpointer-arith \
	-Wcast-qual \
	-Wcast-align \
        -Wshadow \
        -Wwrite-strings \
        -Wredundant-decls"

# remerge the warnings into the FLAGS
CXXFLAGS="$CFLAGS $CXX_WARNINGS"
CFLAGS="$CFLAGS $CC_WARNINGS"

# pass down to the rest of the files.
AC_SUBST(ac_configure_args)

AC_CONFIG_FILES([Makefile 
   sblim-indication_helper.spec \
   test/Makefile])

AC_OUTPUT

