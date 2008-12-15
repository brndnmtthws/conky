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

INCLUDES = -DSYSTEM_CONFIG_FILE=\"$(sysconfdir)/conky/conky.conf\"

bin_PROGRAMS = conky

if BUILD_CONFIG_OUTPUT
config_output = defconfig.h
if BUILD_FOR_FOPENCOOKIE
config_cookie = conf_cookie.c conf_cookie.h
endif
conky_DEPENDENCIES = $(config_output)
BUILT_SOURCES = $(config_output)
CLEANFILES = $(config_output)
$(config_output): ../doc/conky.conf
	sh ../text2c.sh $< $@ defconfig
endif

if BUILD_AUDACIOUS
audacious = audacious.c audacious.h
endif

if BUILD_BMPX
bmpx = bmpx.c
endif

if BUILD_MPD
mpd = mpd.c libmpdclient.c
endif

if BUILD_MOC
moc = moc.c
endif

if BUILD_XMMS2
xmms2 = xmms2.c
endif

if BUILD_LINUX
linux = linux.c top.c diskio.c users.c
PTHREAD_LIBS =  -lpthread
endif

#if BUILD_SOLARIS
#solaris = solaris.c
#endif

if BUILD_FREEBSD
freebsd = freebsd.c
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
port_monitors = libtcp-portmon.h libtcp-portmon.c \
		tcp-portmon.h tcp-portmon.c
endif

if BUILD_X11
x11 = x11.c
endif

if BUILD_HDDTEMP
hddtemp = hddtemp.c
endif

if BUILD_EVE
eve = eve.c eve.h
endif

if BUILD_RSS
rss = rss.c prss.c prss.h
endif

if BUILD_SMAPI
smapi = smapi.c smapi.h
endif

if BUILD_NVIDIA
nvidia = nvidia.c nvidia.h
endif

conky_SOURCES = 		\
	$(config_output)        \
	$(config_cookie)        \
	$(audacious)            \
	$(bmpx)                 \
	common.c 		\
	conky.c                 \
	conky.h                 \
	$(freebsd)              \
	fs.c 			\
	$(hddtemp)		\
	$(linux) 		\
	$(nvidia)		\
	mail.c 			\
	mixer.c 		\
	$(mpd)                  \
  $(moc) \
	$(netbsd)               \
	$(openbsd)              \
	$(port_monitors)        \
	$(eve)			\
	$(rss)			\
	$(solaris) 		\
	timed_thread.c		\
	timed_thread.h		\
	mboxscan.c		\
	mboxscan.h		\
	$(x11)                  \
	$(xmms2)		\
	$(smapi)                \
	temphelper.c            \
	temphelper.h

conky_LDFLAGS = 				\
	$(PTHREAD_LIBS)				\
	-lm

EXTRA_DIST = 			\
	$(config_output)        \
	$(config_cookie)        \
	audacious.c		\
	audacious.h		\
	bmpx.c			\
	common.h		\
	freebsd.c		\
	freebsd.h		\
	fs.h			\
	hddtemp.c		\
	hddtemp.h		\
	linux.c			\
	linux.h			\
	libmpdclient.c          \
	libmpdclient.h          \
	libtcp-portmon.c	\
	libtcp-portmon.h	\
	rss.h			\
	mail.h			\
	mixer.h			\
	moc.h			\
	moc.c			\
	mpd.c			\
	mpd.h			\
	netbsd.c		\
	nvidia.c		\
	nvidia.h		\
	openbsd.c		\
	openbsd.h		\
	solaris.c 		\
	top.h                   \
	diskio.h                \
	x11.c			\
	x11.h			\
	xmms2.c                 \
	xmms2.h                 \
	smapi.c			\
	users.c


# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
