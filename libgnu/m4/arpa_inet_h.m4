# arpa_inet_h.m4 serial 1
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Simon Josefsson

AC_DEFUN([gl_HEADER_ARPA_INET],
[
  AC_CHECK_HEADERS_ONCE([arpa/inet.h])
  if test $ac_cv_header_arpa_inet_h = yes; then
    ARPA_INET_H=''
  else
    ARPA_INET_H='arpa/inet.h'
  fi
  AC_SUBST(ARPA_INET_H)
])
