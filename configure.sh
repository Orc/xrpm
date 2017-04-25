#! /bin/sh

. ./configure.inc

AC_INIT xrpm

AC_PROG_CC || exit 1
unset _MK_LIBRARIAN

if [ "IS_BROKEN_CC" ]; then
    case "$AC_CC $AC_CFLAGS" in
    *-pedantic*) ;;
    *)  # hack around deficiencies in gcc and clang
	#
	AC_DEFINE 'while(x)' 'while( (x) != 0 )'
	AC_DEFINE 'if(x)' 'if( (x) != 0 )'

	if [ "$IS_CLANG" -o "$IS_GCC" ]; then
	    AC_CC="$AC_CC -Wno-return-type -Wno-implicit-int"
	fi ;;
    esac
fi

TLOGN "Checking for system x_getopt"
need_local_getopt=T
if AC_QUIET AC_CHECK_HEADERS basis/options.h; then
    if AC_QUIET AC_CHECK_FUNCS x_getopt; then
	unset need_local_getopt
    elif LIBS="$AC_LIBS -lbasis" AC_QUIET AC_CHECK_FUNCS x_getopt; then
	AC_LIBS="$AC_LIBS -lbasis"
	unset need_local_getopt
    fi
fi

if [ "$need_local_getopt" ]; then
    TLOG " (not found)"
    AC_SUB OPTIONS basis/options.c
    AC_CFLAGS="$AC_CFLAGS -I${AC_SRCDIR}"
else
    TLOG " (found)"
    AC_SUB OPTIONS ''
fi

AC_CHECK_HEADERS errno.h
test "$OS_FREEBSD" || AC_CHECK_HEADERS malloc.h
AC_CHECK_FUNCS	tell || AC_DEFINE 'tell(x)' 'lseek(x,0,SEEK_CUR)'
AC_SCALAR_TYPES

AC_CHECK_FUNCS mkstemp
AC_CHECK_FUNCS getcwd	# so makepkg can set BUILDROOT without depending on parent $PWD

AC_SUB VERSION `cat VERSION`

AC_CHECK_FIELD utsname domainname sys/utsname.h

AC_OUTPUT Makefile xrpm.build
