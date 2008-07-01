/* inet_ntop.c -- convert IPv4 and IPv6 addresses from binary to text form

   Copyright (C) 2005, 2006  Free Software Foundation, Inc.

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

/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <config.h>

/* Specification.  */
#include "inet_ntop.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef EAFNOSUPPORT
# define EAFNOSUPPORT EINVAL
#endif

#define NS_IN6ADDRSZ 16
#define NS_INT16SZ 2

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */
typedef int verify_int_size[2 * sizeof (int) - 7];

static const char *inet_ntop4 (const unsigned char *src, char *dst, socklen_t size);
#if HAVE_IPV6
static const char *inet_ntop6 (const unsigned char *src, char *dst, socklen_t size);
#endif


/* char *
 * inet_ntop(af, src, dst, size)
 *	convert a network format address to presentation format.
 * return:
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *	Paul Vixie, 1996.
 */
const char *
inet_ntop (int af, const void *restrict src,
	   char *restrict dst, socklen_t cnt)
{
  switch (af)
    {
#if HAVE_IPV4
    case AF_INET:
      return (inet_ntop4 (src, dst, cnt));
#endif

#if HAVE_IPV6
    case AF_INET6:
      return (inet_ntop6 (src, dst, cnt));
#endif

    default:
      errno = EAFNOSUPPORT;
      return (NULL);
    }
  /* NOTREACHED */
}

/* const char *
 * inet_ntop4(src, dst, size)
 *	format an IPv4 address
 * return:
 *	`dst' (as a const)
 * notes:
 *	(1) uses no statics
 *	(2) takes a u_char* not an in_addr as input
 * author:
 *	Paul Vixie, 1996.
 */
static const char *
inet_ntop4 (const unsigned char *src, char *dst, socklen_t size)
{
  char tmp[sizeof "255.255.255.255"];
  int len;

  len = sprintf (tmp, "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
  if (len < 0)
    return NULL;

  if (len > size)
    {
      errno = ENOSPC;
      return NULL;
    }

  return strcpy (dst, tmp);
}

#if HAVE_IPV6

/* const char *
 * inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * author:
 *	Paul Vixie, 1996.
 */
static const char *
inet_ntop6 (const unsigned char *src, char *dst, socklen_t size)
{
  /*
   * Note that int32_t and int16_t need only be "at least" large enough
   * to contain a value of the specified size.  On some systems, like
   * Crays, there is no such thing as an integer variable with 16 bits.
   * Keep this in mind if you think this function should have been coded
   * to use pointer overlays.  All the world's not a VAX.
   */
  char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
  struct
  {
    int base, len;
  } best, cur;
  unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
  int i;

  /*
   * Preprocess:
   *      Copy the input (bytewise) array into a wordwise array.
   *      Find the longest run of 0x00's in src[] for :: shorthanding.
   */
  memset (words, '\0', sizeof words);
  for (i = 0; i < NS_IN6ADDRSZ; i += 2)
    words[i / 2] = (src[i] << 8) | src[i + 1];
  best.base = -1;
  cur.base = -1;
  for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
    {
      if (words[i] == 0)
	{
	  if (cur.base == -1)
	    cur.base = i, cur.len = 1;
	  else
	    cur.len++;
	}
      else
	{
	  if (cur.base != -1)
	    {
	      if (best.base == -1 || cur.len > best.len)
		best = cur;
	      cur.base = -1;
	    }
	}
    }
  if (cur.base != -1)
    {
      if (best.base == -1 || cur.len > best.len)
	best = cur;
    }
  if (best.base != -1 && best.len < 2)
    best.base = -1;

  /*
   * Format the result.
   */
  tp = tmp;
  for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
    {
      /* Are we inside the best run of 0x00's? */
      if (best.base != -1 && i >= best.base && i < (best.base + best.len))
	{
	  if (i == best.base)
	    *tp++ = ':';
	  continue;
	}
      /* Are we following an initial run of 0x00s or any real hex? */
      if (i != 0)
	*tp++ = ':';
      /* Is this address an encapsulated IPv4? */
      if (i == 6 && best.base == 0 &&
	  (best.len == 6 || (best.len == 5 && words[5] == 0xffff)))
	{
	  if (!inet_ntop4 (src + 12, tp, sizeof tmp - (tp - tmp)))
	    return (NULL);
	  tp += strlen (tp);
	  break;
	}
      {
	int len = sprintf (tp, "%x", words[i]);
	if (len < 0)
	  return NULL;
	tp += len;
      }
    }
  /* Was it a trailing run of 0x00's? */
  if (best.base != -1 && (best.base + best.len) ==
      (NS_IN6ADDRSZ / NS_INT16SZ))
    *tp++ = ':';
  *tp++ = '\0';

  /*
   * Check for overflow, copy, and we're done.
   */
  if ((socklen_t) (tp - tmp) > size)
    {
      errno = ENOSPC;
      return NULL;
    }

  return strcpy (dst, tmp);
}

#endif
