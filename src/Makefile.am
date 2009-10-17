# -*- mode: Makefile; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
#
# Conky, a system monitor, based on torsmo
#
# Any original torsmo code is licensed under the BSD license
#
# All code written since the fork of torsmo is licensed under the GPL
#
# Please see COPYING for details
#
# Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
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

INCLUDES = -DSYSTEM_CONFIG_FILE=\"$(sysconfdir)/conky/conky.conf\" -DPACKAGE_LIBDIR=\"$(pkglibdir)\"

bin_PROGRAMS = conky

LIBS = ${conky_LIBS}

if BUILD_CONFIG_OUTPUT
config_output = defconfig.h
config_cookie = conf_cookie.c conf_cookie.h
if BUILD_X11
config_input = $(srcdir)/../data/conky.conf
else
config_input = $(srcdir)/../data/conky_no_x11.conf
endif

conky_DEPENDENCIES = $(config_output)
BUILT_SOURCES = $(config_output)
CLEANFILES = $(config_output)

$(config_output): ${config_input}
	sh $(srcdir)/../text2c.sh ${config_input} $@ defconfig
endif # BUILD_CONFIG_OUTPUT

# source files always needed for compiling
mandatory_sources = colours.c colours.h common.c common.h conky.c conky.h \
		core.c core.h diskio.c diskio.h exec.c exec.h fs.c fs.h logging.h \
		mail.c mail.h mixer.c mixer.h template.c template.h timed_thread.c \
		timed_thread.h mboxscan.c mboxscan.h read_tcp.c read_tcp.h specials.c \
		specials.h tailhead.c tailhead.h temphelper.c temphelper.h \
		text_object.c text_object.h timeinfo.c timeinfo.h algebra.c algebra.h

# source files only needed when the apropriate option is enabled
audacious = audacious.c audacious.h
bmpx = bmpx.c bmpx.h
ibm = ibm.c ibm.h smapi.c smapi.h
mpd = mpd.c mpd.h libmpdclient.c libmpdclient.h
moc = moc.c moc.h
xmms2 = xmms2.c xmms2.h
linux = linux.c linux.h top.c top.h users.c sony.c sony.h
solaris = solaris.c
freebsd = freebsd.c freebsd.h
netbsd = netbsd.c netbsd.h
openbsd = openbsd.c openbsd.h
port_monitors = libtcp-portmon.c libtcp-portmon.h \
                tcp-portmon.c tcp-portmon.h
x11 = x11.c x11.h fonts.c fonts.h
hddtemp = hddtemp.c hddtemp.h
eve = eve.c eve.h
ccurl_thread = ccurl_thread.c ccurl_thread.h
rss = rss.c rss.h prss.c prss.h
weather = weather.c weather.h
lua = llua.c llua.h
nvidia = nvidia.c nvidia.h
imlib2 = imlib2.c imlib2.h
apcupsd = apcupsd.c apcupsd.h
iconv = iconv_tools.c iconv_tools.h

# make sure the files from above are always included in the distfile
EXTRA_DIST = $(audacious) $(bmpx) $(ibm) $(mpd) $(moc) $(xmms2) $(linux) \
             $(solaris) $(freebsd) $(netbsd) $(openbsd) $(port_monitors) \
             $(x11) $(hddtemp) $(eve) $(ccurl_thread) $(rss) $(weather) \
             $(lua) $(nvidia) $(imlib2) $(apcupsd)

# collect all selected optional sources
optional_sources =
if BUILD_AUDACIOUS
optional_sources += $(audacious)
endif
if BUILD_BMPX
optional_sources += $(bmpx)
endif
if BUILD_IBM
optional_sources += $(ibm)
endif
if BUILD_MPD
optional_sources += $(mpd)
endif
if BUILD_MOC
optional_sources += $(moc)
endif
if BUILD_XMMS2
optional_sources += $(xmms2)
endif
if BUILD_LINUX
optional_sources += $(linux)
endif
#if BUILD_SOLARIS
#optional_sources += $(solaris)
#endif
if BUILD_FREEBSD
optional_sources += $(freebsd)
endif
#if BUILD_NETBSD
#optional_sources += $(netbsd)
#endif
if BUILD_OPENBSD
optional_sources += $(openbsd)
endif
if BUILD_PORT_MONITORS
optional_sources += $(port_monitors)
endif
if BUILD_X11
optional_sources += $(x11)
endif
if BUILD_HDDTEMP
optional_sources += $(hddtemp)
endif
if BUILD_EVE
optional_sources += $(eve)
endif
if BUILD_CURL
optional_sources += $(ccurl_thread)
endif
if BUILD_RSS
optional_sources += $(rss)
endif
if BUILD_WEATHER
optional_sources += $(weather)
endif
if BUILD_LUA
optional_sources += $(lua)
endif
if BUILD_NVIDIA
optional_sources += $(nvidia)
endif
if BUILD_IMLIB2
optional_sources += $(imlib2)
endif
if BUILD_APCUPSD
optional_sources += $(apcupsd)
endif
if BUILD_ICONV
optional_sources += $(iconv)
endif

# linux takes the standard to the max
if BUILD_LINUX
PTHREAD_LIBS = -lpthread
else
PTHREAD_LIBS = -pthread
endif

conky_LDFLAGS = $(PTHREAD_LIBS) -lm

conky_SOURCES = $(config_output) $(config_cookie) \
		$(optional_sources) $(mandatory_sources)

# vi:set ts=4 sw=4 noet ai nocindent syntax=automake:
