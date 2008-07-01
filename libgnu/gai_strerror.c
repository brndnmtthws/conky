/* Copyright (C) 1997, 2001, 2002, 2004, 2005, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Philip Blundell <pjb27@cam.ac.uk>, 1997.

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

#ifndef _LIBC
# include <config.h>
# include "getaddrinfo.h"
#endif

#include <stdio.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef _LIBC
# include <libintl.h>
#else
# include "gettext.h"
# define _(String) gettext (String)
# define N_(String) String
#endif

static struct
  {
    int code;
    const char *msg;
  }
values[] =
  {
    { EAI_ADDRFAMILY, N_("Address family for hostname not supported") },
    { EAI_AGAIN, N_("Temporary failure in name resolution") },
    { EAI_BADFLAGS, N_("Bad value for ai_flags") },
    { EAI_FAIL, N_("Non-recoverable failure in name resolution") },
    { EAI_FAMILY, N_("ai_family not supported") },
    { EAI_MEMORY, N_("Memory allocation failure") },
    { EAI_NODATA, N_("No address associated with hostname") },
    { EAI_NONAME, N_("Name or service not known") },
    { EAI_SERVICE, N_("Servname not supported for ai_socktype") },
    { EAI_SOCKTYPE, N_("ai_socktype not supported") },
    { EAI_SYSTEM, N_("System error") },
    { EAI_OVERFLOW, N_("Argument buffer too small") },
#ifdef __USE_GNU
    { EAI_INPROGRESS, N_("Processing request in progress") },
    { EAI_CANCELED, N_("Request canceled") },
    { EAI_NOTCANCELED, N_("Request not canceled") },
    { EAI_ALLDONE, N_("All requests done") },
    { EAI_INTR, N_("Interrupted by a signal") },
    { EAI_IDN_ENCODE, N_("Parameter string not correctly encoded") }
#endif
  };

const char *
gai_strerror (int code)
{
  size_t i;
  for (i = 0; i < sizeof (values) / sizeof (values[0]); ++i)
    if (values[i].code == code)
      return _(values[i].msg);

  return _("Unknown error");
}
#ifdef _LIBC
libc_hidden_def (gai_strerror)
#endif
