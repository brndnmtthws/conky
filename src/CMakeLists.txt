# vim: ts=4 sw=4 noet ai cindent syntax=cmake
#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2012 Brenden Matthews, et. al. (see AUTHORS)
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

include_directories(${conky_includes})
if(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)
	# include config output dir
	include_directories(${CMAKE_BINARY_DIR}/data)
endif(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)

# ensure build.h and config.h aren't in the way
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
	message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/config.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)
	message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/build.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h)
	message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h)

set(conky_sources c++wrap.cc colours.cc combine.cc common.cc conky.cc core.cc
	diskio.cc entropy.cc exec.cc fs.cc mail.cc mixer.cc net_stat.cc template.cc
	mboxscan.cc read_tcpip.cc scroll.cc specials.cc tailhead.cc temphelper.cc
	text_object.cc timeinfo.cc top.cc algebra.cc prioqueue.cc proc.cc user.cc
	luamm.cc data-source.cc lua-config.cc setting.cc llua.cc update-cb.cc)

# Platform specific sources
if(OS_LINUX)
	set(linux linux.cc users.cc sony.cc i8k.cc)
	set(optional_sources ${optional_sources} ${linux})
endif(OS_LINUX)

if(OS_FREEBSD)
	set(freebsd freebsd.cc bsdapm.cc)
	set(optional_sources ${optional_sources} ${freebsd})
endif(OS_FREEBSD)

if(OS_DRAGONFLY)
    set(dragonfly dragonfly.cc bsdapm.cc)
    set(optional_sources ${optional_sources} ${dragonfly})
endif(OS_DRAGONFLY)

if(OS_OPENBSD)
	set(openbsd openbsd.cc bsdapm.cc)
	set(optional_sources ${optional_sources} ${openbsd})
endif(OS_OPENBSD)

# These below are not actually supported.  No idea what their status is.
if(OS_SOLARIS)
	set(solaris solaris.cc)
	set(optional_sources ${optional_sources} ${solaris})
endif(OS_SOLARIS)

if(OS_NETBSD)
	set(netbsd netbsd.cc)
	set(optional_sources ${optional_sources} ${netbsd})
endif(OS_NETBSD)


# Optional sources
if(BUILD_AUDACIOUS)
	set(audacious audacious.cc)
	set(optional_sources ${optional_sources} ${audacious})
endif(BUILD_AUDACIOUS)

if(BUILD_BMPX)
	set(bmpx bmpx.cc)
	set(optional_sources ${optional_sources} ${bmpx})
endif(BUILD_BMPX)

if(BUILD_IBM)
	set(ibm ibm.cc smapi.cc)
	set(optional_sources ${optional_sources} ${ibm})
endif(BUILD_IBM)

if(BUILD_MPD)
	set(mpd mpd.cc libmpdclient.cc)
	set(optional_sources ${optional_sources} ${mpd})
endif(BUILD_MPD)

if(BUILD_MYSQL)
	set(mysql mysql.cc)
	set(optional_sources ${optional_sources} ${mysql})
endif(BUILD_MYSQL)

if(BUILD_MOC)
	set(moc moc.cc)
	set(optional_sources ${optional_sources} ${moc})
endif(BUILD_MOC)

if(BUILD_CMUS)
       set(cmus cmus.cc)
       set(optional_sources ${optional_sources} ${cmus})
endif(BUILD_CMUS)

if(BUILD_XMMS2)
	set(xmms2 xmms2.cc)
	set(optional_sources ${optional_sources} ${xmms2})
endif(BUILD_XMMS2)

if(BUILD_PORT_MONITORS)
	add_library(tcp-portmon libtcp-portmon.cc)
	set(conky_libs ${conky_libs} tcp-portmon)
	set(port_monitors tcp-portmon.cc)
	set(optional_sources ${optional_sources} ${port_monitors})
endif(BUILD_PORT_MONITORS)

if(BUILD_X11)
	set(x11 x11.cc fonts.cc)
	set(optional_sources ${optional_sources} ${x11})
endif(BUILD_X11)

if(BUILD_HDDTEMP)
	set(hddtemp hddtemp.cc)
	set(optional_sources ${optional_sources} ${hddtemp})
endif(BUILD_HDDTEMP)

if(BUILD_EVE)
	set(eve eve.cc)
	set(optional_sources ${optional_sources} ${eve})
endif(BUILD_EVE)

if(BUILD_CURL)
	set(ccurl_thread ccurl_thread.cc)
	set(optional_sources ${optional_sources} ${ccurl_thread})
endif(BUILD_CURL)

if(BUILD_RSS)
	set(rss rss.cc prss.cc)
	set(optional_sources ${optional_sources} ${rss})
endif(BUILD_RSS)

if(BUILD_WEATHER)
	set(weather weather.cc)
	set(optional_sources ${optional_sources} ${weather})
endif(BUILD_WEATHER)

if(BUILD_NVIDIA)
	set(nvidia nvidia.cc)
	set(optional_sources ${optional_sources} ${nvidia})
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
	set(imlib2 imlib2.cc)
	set(optional_sources ${optional_sources} ${imlib2})
endif(BUILD_IMLIB2)

if(BUILD_APCUPSD)
	set(apcupsd apcupsd.cc)
	set(optional_sources ${optional_sources} ${apcupsd})
endif(BUILD_APCUPSD)

if(BUILD_ICAL)
	set(ical ical.cc)
	set(optional_sources ${optional_sources} ${ical})
endif(BUILD_ICAL)

if(BUILD_IRC)
	set(irc irc.cc)
	set(optional_sources ${optional_sources} ${irc})
endif(BUILD_IRC)

if(BUILD_ICONV)
	set(iconv iconv_tools.cc)
	set(optional_sources ${optional_sources} ${iconv})
endif(BUILD_ICONV)

if(BUILD_NCURSES)
	 set(optional_sources ${optional_sources} nc.cc)
endif(BUILD_NCURSES)

add_executable(conky ${conky_sources} ${optional_sources})

target_link_libraries(conky ${conky_libs})

# Install libtcp-portmon too?
install(TARGETS
	conky
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
