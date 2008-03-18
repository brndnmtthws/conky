/* smapi.h:  conky support for IBM Thinkpad smapi
 *
 * Copyright (C) 2007 Phil Sutter <Phil@nwl.cc>
 * 
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

int smapi_bat_installed(int);

char *smapi_read_str(char *);
int smapi_read_int(char *);

char *smapi_get_str(char *);
char *smapi_get_val(char *);

char *smapi_get_bat_str(int, char *);
int smapi_get_bat_int(int, char *);
char *smapi_get_bat_val(char *);
