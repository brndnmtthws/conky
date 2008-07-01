# stdlib_h.m4 serial 3
dnl Copyright (C) 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STDLIB_H],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  gl_CHECK_NEXT_HEADERS([stdlib.h])
])

AC_DEFUN([gl_STDLIB_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  GNULIB_[]m4_translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___])=1
])

AC_DEFUN([gl_STDLIB_H_DEFAULTS],
[
  GNULIB_MALLOC_POSIX=0;  AC_SUBST([GNULIB_MALLOC_POSIX])
  GNULIB_REALLOC_POSIX=0; AC_SUBST([GNULIB_REALLOC_POSIX])
  GNULIB_CALLOC_POSIX=0;  AC_SUBST([GNULIB_CALLOC_POSIX])
  GNULIB_GETSUBOPT=0;     AC_SUBST([GNULIB_GETSUBOPT])
  GNULIB_MKDTEMP=0;       AC_SUBST([GNULIB_MKDTEMP])
  GNULIB_MKSTEMP=0;       AC_SUBST([GNULIB_MKSTEMP])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_CALLOC_POSIX=1;    AC_SUBST([HAVE_CALLOC_POSIX])
  HAVE_GETSUBOPT=1;       AC_SUBST([HAVE_GETSUBOPT])
  HAVE_MALLOC_POSIX=1;    AC_SUBST([HAVE_MALLOC_POSIX])
  HAVE_MKDTEMP=1;         AC_SUBST([HAVE_MKDTEMP])
  HAVE_REALLOC_POSIX=1;   AC_SUBST([HAVE_REALLOC_POSIX])
  REPLACE_MKSTEMP=0;      AC_SUBST([REPLACE_MKSTEMP])
])
