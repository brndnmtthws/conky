# sys_socket_h.m4 serial 4
dnl Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Simon Josefsson.

AC_DEFUN([gl_HEADER_SYS_SOCKET],
[
  AC_CACHE_CHECK([whether <sys/socket.h> is self-contained],
    [gl_cv_header_sys_socket_h_selfcontained],
    [
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <sys/socket.h>], [])],
        [gl_cv_header_sys_socket_h_selfcontained=yes],
        [gl_cv_header_sys_socket_h_selfcontained=no])
    ])
  if test $gl_cv_header_sys_socket_h_selfcontained = yes; then
    SYS_SOCKET_H=''
  else
    SYS_SOCKET_H='sys/socket.h'

    gl_CHECK_NEXT_HEADERS([sys/socket.h])
    if test $ac_cv_header_sys_socket_h = yes; then
      HAVE_SYS_SOCKET_H=1
      HAVE_WINSOCK2_H=0
      HAVE_WS2TCPIP_H=0
    else
      HAVE_SYS_SOCKET_H=0
      dnl We cannot use AC_CHECK_HEADERS_ONCE here, because that would make
      dnl the check for those headers unconditional; yet cygwin reports
      dnl that the headers are present but cannot be compiled (since on
      dnl cygwin, all socket information should come from sys/socket.h).
      AC_CHECK_HEADERS([winsock2.h ws2tcpip.h])
      if test $ac_cv_header_winsock2_h = yes; then
        HAVE_WINSOCK2_H=1
      else
        HAVE_WINSOCK2_H=0
      fi
      if test $ac_cv_header_ws2tcpip_h = yes; then
        HAVE_WS2TCPIP_H=1
      else
        HAVE_WS2TCPIP_H=0
      fi
    fi
    AC_SUBST([HAVE_SYS_SOCKET_H])
    AC_SUBST([HAVE_WINSOCK2_H])
    AC_SUBST([HAVE_WS2TCPIP_H])
  fi
  AC_SUBST([SYS_SOCKET_H])
])
