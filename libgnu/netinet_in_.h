/* Substitute for <netinet/in.h>.
   Copyright (C) 2007 Free Software Foundation, Inc.

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

#ifndef _GL_NETINET_IN_H

#if @HAVE_NETINET_IN_H@

/* On many platforms, <netinet/in.h> assumes prior inclusion of
   <sys/types.h>.  */
# include <sys/types.h>

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_NETINET_IN_H@

#endif

#ifndef _GL_NETINET_IN_H
#define _GL_NETINET_IN_H

#if !@HAVE_NETINET_IN_H@

/* A platform that lacks <netinet/in.h>.  */

# include <sys/socket.h>

#endif

#endif /* _GL_NETINET_IN_H */
#endif /* _GL_NETINET_IN_H */
