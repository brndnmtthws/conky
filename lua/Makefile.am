# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

libimlib2_la_SOURCES = imlib2.c
libcairo_la_SOURCES = cairo.c

EXTRA_DIST = \
			 cairo.pkg \
			 imlib2.pkg

clean-am: clean-generic mostlyclean-am
	rm -f $(libimlib2_la_SOURCES)
	rm -f $(libcairo_la_SOURCES)
	rm -f *.la *.so

if BUILD_LUA 

if BUILD_LUA_CAIRO
libcairo = libcairo.la

libcairo_la_CFLAGS = $(cairo_CFLAGS) $(cairo_xlib_CFLAGS) -I$(prefix)/include/lua5.1
libcairo_la_LIBADD = $(cairo_LIBS) $(cairo_xlib_LIBS) $(tolua_LIBS)

cairo.c: cairo.pkg
	${toluapp} -n cairo -o cairo.c $(srcdir)/cairo.pkg

endif # BUILD_LUA_CAIRO

if BUILD_LUA_IMLIB2

libimlib2 = libimlib2.la

libimlib2_la_CFLAGS = $(Imlib2_CFLAGS) -I$(prefix)/include/lua5.1
libimlib2_la_LIBADD = $(Imlib2_LIBS) $(tolua_LIBS)

imlib2.c: imlib2.pkg
	${toluapp} -n imlib2 -o imlib2.c $(srcdir)/imlib2.pkg

endif # BUILD_LUA_IMLIB2
pkglib_LTLIBRARIES = $(libcairo) $(libimlib2)

endif # BUILD_LUA

