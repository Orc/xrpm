#! /bin/sh

. ./configure.inc

AC_INIT xrpm

AC_PROG_CC || exit 1

unset _MK_LIBRARIAN

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
AC_CHECK_FUNCS	tell
AC_SCALAR_TYPES

AC_CHECK_HEADERS arpa/inet.h

AC_CHECK_FIELD utsname domainname sys/utsname.h

AC_OUTPUT Makefile
