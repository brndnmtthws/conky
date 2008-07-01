# getaddrinfo.m4 serial 12
dnl Copyright (C) 2004, 2005, 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GETADDRINFO],
[
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])dnl for HAVE_SYS_SOCKET_H, HAVE_WINSOCK2_H
  AC_MSG_NOTICE([checking how to do getaddrinfo, freeaddrinfo and getnameinfo])

  AC_SEARCH_LIBS(getaddrinfo, [nsl socket])
  AC_CHECK_FUNCS(getaddrinfo,, [
    AC_CACHE_CHECK(for getaddrinfo in ws2tcpip.h and -lws2_32,
		   gl_cv_w32_getaddrinfo, [
      gl_cv_w32_getaddrinfo=no
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -lws2_32"
      AC_TRY_LINK([
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
], [getaddrinfo(0, 0, 0, 0);], gl_cv_w32_getaddrinfo=yes)
    LIBS="$am_save_LIBS"])
    if test "$gl_cv_w32_getaddrinfo" = "yes"; then
      LIBS="$LIBS -lws2_32"
    else
      AC_LIBOBJ(getaddrinfo)
    fi
    ])

  # We can't use AC_REPLACE_FUNCS here because gai_strerror may be an
  # inline function declared in ws2tcpip.h, so we need to get that
  # header included somehow.
  AC_CHECK_HEADERS_ONCE(netdb.h)
  AC_CACHE_CHECK([for gai_strerror (possibly via ws2tcpip.h)],
    gl_cv_func_gai_strerror, [
      AC_TRY_LINK([
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
], [gai_strerror (0);],
        [gl_cv_func_gai_strerror=yes],
        [gl_cv_func_gai_strerror=no])])
  if test $gl_cv_func_gai_strerror = no; then
    AC_LIBOBJ(gai_strerror)
  fi

  gl_PREREQ_GETADDRINFO
])

# Prerequisites of lib/getaddrinfo.h and lib/getaddrinfo.c.
AC_DEFUN([gl_PREREQ_GETADDRINFO], [
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])dnl for HAVE_SYS_SOCKET_H, HAVE_WINSOCK2_H
  AC_SEARCH_LIBS(gethostbyname, [inet nsl])
  AC_SEARCH_LIBS(getservbyname, [inet nsl socket xnet])
  AC_CHECK_FUNCS(gethostbyname,, [
    AC_CACHE_CHECK(for gethostbyname in winsock2.h and -lws2_32,
		   gl_cv_w32_gethostbyname, [
      gl_cv_w32_gethostbyname=no
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -lws2_32"
      AC_TRY_LINK([
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
], [gethostbyname(0);], gl_cv_w32_gethostbyname=yes)
    LIBS="$am_save_LIBS"])
    if test "$gl_cv_w32_gethostbyname" = "yes"; then
      LIBS="$LIBS -lws2_32"
    fi
    ])
  AC_REQUIRE([AC_C_RESTRICT])
  AC_REQUIRE([gl_SOCKET_FAMILIES])
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_GNU_SOURCE])
  AC_CHECK_HEADERS_ONCE(netinet/in.h netdb.h)
  AC_CHECK_DECLS([getaddrinfo, freeaddrinfo, gai_strerror, getnameinfo],,,[
  /* sys/types.h is not needed according to POSIX, but the
     sys/socket.h in i386-unknown-freebsd4.10 and
     powerpc-apple-darwin5.5 required it. */
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
])
  AC_CHECK_TYPES([struct addrinfo],,,[
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
])
])
