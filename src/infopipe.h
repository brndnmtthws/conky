/* -------------------------------------------------------------------------
 * infopipe.h:  conky support for XMMS/BMP InfoPipe plugin
 * 
 * InfoPipe: http://www.beastwithin.org/users/wwwwolf/code/xmms/infopipe.html
 *
 * Copyright (C) 2005  Philip Kovacs kovacsp3@comcast.net
 *
 * Based on original ideas and code graciously presented by:
 * Ulrich Jansen - ulrich( dot )jansen( at )rwth-aachen.de
 *
 * $Id$
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * --------------------------------------------------------------------------- */

#ifndef INFOPIPE_H
#define INFOPIPE_H

#define INFOPIPE_NAMED_PIPE "/tmp/xmms-info"

void update_infopipe(void);
void *infopipe_service(void *);

#endif
