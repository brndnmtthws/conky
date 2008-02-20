# Conky, a system monitor, based on torsmo
#
# Any original torsmo code is licensed under the BSD license
#
# All code written since the fork of torsmo is licensed under the GPL
#
# Please see COPYING for details
#
# Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
# Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
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
# $Id$

INCLUDES =			\
	-DSYSTEM_CONFIG_FILE=\"$(sysconfdir)/conky/conky.conf\"

bin_PROGRAMS = conky

if BUILD_AUDACIOUS
audacious = audacious.c audacious.h
endif

if BUILD_BMPX
bmpx = bmpx.c
endif

if BUILD_MPD
mpd = mpd.c libmpdclient.c
endif

if BUILD_XMMS2
xmms2 = xmms2.c
endif

if BUILD_LINUX
linux = linux.c top.c diskio.c
PTHREAD_LIBS =  -lpthread
endif

#if BUILD_SOLARIS
#solaris = solaris.c
#endif

if BUILD_FREEBSD
freebsd = freebsd.c diskio.c
PTHREAD_LIBS =  -pthread
endif

#if BUILD_NETBSD
#netbsd = netbsd.c
#endif

if BUILD_OPENBSD
openbsd = openbsd.c
PTHREAD_LIBS =  -pthread
endif

if BUILD_PORT_MONITORS
port_monitors = libtcp-portmon.h libtcp-portmon.c
endif

if BUILD_X11
x11 = x11.c
endif

if BUILD_HDDTEMP
hddtemp = hddtemp.c
endif

if BUILD_RSS
rss = rss.c prss.c prss.h
endif

conky_SOURCES = 		\
	$(audacious)            \
	$(bmpx)                 \
	common.c 		\
	conky.c                 \
	conky.h                 \
	$(freebsd)              \
	fs.c 			\
	$(hddtemp)		\
	$(linux) 		\
	mail.c 			\
	mixer.c 		\
	$(mpd)                  \
	$(netbsd)               \
	$(openbsd)              \
	$(port_monitors)        \
	$(rss)			\
	$(solaris) 		\
	timed_thread.c		\
	timed_thread.h		\
	mboxscan.c		\
	mboxscan.h		\
	$(x11)                  \
	$(xmms2)

AM_LDFLAGS = $(PTHREAD_LIBS) -lm

EXTRA_DIST = 			\
	audacious.c		\
	audacious.h		\
	bmpx.c			\
	freebsd.c		\
	hddtemp.c		\
	linux.c			\
	libmpdclient.c          \
	libmpdclient.h          \
	libtcp-portmon.c	\
	libtcp-portmon.h	\
	mpd.c			\
	netbsd.c		\
	openbsd.c		\
	solaris.c 		\
	top.h                   \
	diskio.h                \
	x11.c                   \
	xmms2.c

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
