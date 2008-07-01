/* Provide a sys/socket header file for systems lacking it (read: MinGW).
   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
   Written by Simon Josefsson.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* This file is supposed to be used on platforms that lack <sys/socket.h>
   and on platforms where <sys/socket.h> cannot be included standalone.
   It is intended to provide definitions and prototypes needed by an
   application.  */

#ifndef _GL_SYS_SOCKET_H

#if @HAVE_SYS_SOCKET_H@

/* On many platforms, <sys/socket.h> assumes prior inclusion of
   <sys/types.h>.  */
# include <sys/types.h>

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_SYS_SOCKET_H@

#endif

#ifndef _GL_SYS_SOCKET_H
#define _GL_SYS_SOCKET_H

#if !@HAVE_SYS_SOCKET_H@

/* A platform that lacks <sys/socket.h>.

   Currently only MinGW is supported.  See the gnulib manual regarding
   Windows sockets.  MinGW has the header files winsock2.h and
   ws2tcpip.h that declare the sys/socket.h definitions we need.  Note
   that you can influence which definitions you get by setting the
   WINVER symbol before including these two files.  For example,
   getaddrinfo is only available if _WIN32_WINNT >= 0x0501 (that
   symbol is set indiriectly through WINVER).  You can set this by
   adding AC_DEFINE(WINVER, 0x0501) to configure.ac.  Note that your
   code may not run on older Windows releases then.  My Windows 2000
   box was not able to run the code, for example.  The situation is
   slightly confusing because:
   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/getaddrinfo_2.asp
   suggests that getaddrinfo should be available on all Windows
   releases. */


# if @HAVE_WINSOCK2_H@
#  include <winsock2.h>
# endif
# if @HAVE_WS2TCPIP_H@
#  include <ws2tcpip.h>
# endif

/* For shutdown(). */
# if !defined SHUT_RD && defined SD_RECEIVE
#  define SHUT_RD SD_RECEIVE
# endif
# if !defined SHUT_WR && defined SD_SEND
#  define SHUT_WR SD_SEND
# endif
# if !defined SHUT_RDWR && defined SD_BOTH
#  define SHUT_RDWR SD_BOTH
# endif

# if defined _WIN32 || defined __WIN32__
#  define ENOTSOCK                WSAENOTSOCK
#  define EADDRINUSE              WSAEADDRINUSE
#  define ENETRESET               WSAENETRESET
#  define ECONNABORTED            WSAECONNABORTED
#  define ECONNRESET              WSAECONNRESET
#  define ENOTCONN                WSAENOTCONN
#  define ESHUTDOWN               WSAESHUTDOWN
# endif

#endif /* HAVE_SYS_SOCKET_H */

#endif /* _GL_SYS_SOCKET_H */
#endif /* _GL_SYS_SOCKET_H */
