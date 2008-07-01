dnl A placeholder for ISO C99 <wchar.h>, for platforms that have issues.

dnl Copyright (C) 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Eric Blake.

# wchar.m4 serial 3

AC_DEFUN([gl_WCHAR_H],
[
  AC_CACHE_CHECK([whether <wchar.h> is standalone],
    [gl_cv_header_wchar_h_standalone],
    [AC_COMPILE_IFELSE([[#include <wchar.h>
wchar_t w;]],
      [gl_cv_header_wchar_h_standalone=yes],
      [gl_cv_header_wchar_h_standalone=no])])
  if test $gl_cv_header_wchar_h_standalone = yes; then
    WCHAR_H=
  else
    dnl Check for <wchar.h> (missing in Linux uClibc when built without wide
    dnl character support).
    AC_CHECK_HEADERS_ONCE([wchar.h])
    if test $ac_cv_header_wchar_h = yes; then
      HAVE_WCHAR_H=1
    else
      HAVE_WCHAR_H=0
    fi
    AC_SUBST([HAVE_WCHAR_H])
    gl_CHECK_NEXT_HEADERS([wchar.h])
    WCHAR_H=wchar.h
  fi
  AC_SUBST([WCHAR_H])
])
