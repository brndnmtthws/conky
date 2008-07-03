# netinet_in_h.m4 serial 3
dnl Copyright (C) 2006-2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_HEADER_NETINET_IN],
[
  AC_CACHE_CHECK([whether <netinet/in.h> is self-contained],
    [gl_cv_header_netinet_in_h_selfcontained],
    [
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <netinet/in.h>], [])],
        [gl_cv_header_netinet_in_h_selfcontained=yes],
        [gl_cv_header_netinet_in_h_selfcontained=no])
    ])
  if test $gl_cv_header_netinet_in_h_selfcontained = yes; then
    NETINET_IN_H=''
  else
    NETINET_IN_H='netinet/in.h'
    AC_CHECK_HEADERS([netinet/in.h])
    gl_CHECK_NEXT_HEADERS([netinet/in.h])
    if test $ac_cv_header_netinet_in_h = yes; then
      HAVE_NETINET_IN_H=1
    else
      HAVE_NETINET_IN_H=0
    fi
    AC_SUBST([HAVE_NETINET_IN_H])
  fi
  AC_SUBST([NETINET_IN_H])
])
