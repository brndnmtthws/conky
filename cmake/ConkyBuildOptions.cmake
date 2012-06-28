# vim: ts=4 sw=4 noet ai cindent syntax=cmake
#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2010 Brenden Matthews, et. al. (see AUTHORS)
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

if(NOT CMAKE_BUILD_TYPE)
	if(MAINTAINER_MODE)
		set(CMAKE_BUILD_TYPE Debug CACHE STRING
			"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
			FORCE)
	else(MAINTAINER_MODE)
		set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
			"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
			FORCE)
	endif(MAINTAINER_MODE)
endif(NOT CMAKE_BUILD_TYPE)

# -std options for all build types
set(CMAKE_C_FLAGS "-std=c99" CACHE STRING "Flags used by the C compiler during all build types." FORCE)
set(CMAKE_CXX_FLAGS "-std=c++0x" CACHE STRING "Flags used by the C++ compiler during all build types." FORCE)

if(MAINTAINER_MODE)
	# some extra debug flags when in 'maintainer mode'
	set(CMAKE_C_FLAGS_DEBUG "-ggdb -Wall -W -Wextra -Wunused -Wdeclaration-after-statement -Wundef -Wendif-labels -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Winline -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -pedantic -Werror" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
	set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Wall -W -Wextra -Wunused -pedantic -Werror -Wno-format" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
endif(MAINTAINER_MODE)


option(RELEASE "Build release package" false)
mark_as_advanced(RELEASE)

option(MAINTAINER_MODE "Enable maintainer mode (builds docs)" false)

option(BUILD_I18N  "Enable if you want internationalization support" true)
if(BUILD_I18N)
	set(LOCALE_DIR "${CMAKE_INSTALL_PREFIX}/share/locale" CACHE STRING "Directory containing the locales")
endif(BUILD_I18N)

# Some standard options
set(SYSTEM_CONFIG_FILE "/etc/conky/conky.conf" CACHE STRING "Default system-wide Conky configuration file")
# use FORCE below to make sure this changes when CMAKE_INSTALL_PREFIX is modified
set(PACKAGE_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib/conky" CACHE STRING "Package library path (where Lua bindings are installed" FORCE)
set(DEFAULTNETDEV "eth0" CACHE STRING "Default networkdevice")
set(CONFIG_FILE "$HOME/.conkyrc" CACHE STRING "Configfile of the user")
set(MAX_USER_TEXT_DEFAULT "16384" CACHE STRING "Default maximum size of config TEXT buffer, i.e. below TEXT line.")
set(DEFAULT_TEXT_BUFFER_SIZE "256" CACHE STRING "Default size used for temporary, static text buffers")
set(MAX_NET_INTERFACES "64" CACHE STRING "Maximum number of network devices")


# Platform specific options
# Linux only
if(OS_LINUX)
	option(BUILD_PORT_MONITORS "Build TCP portmon support" true)
	option(BUILD_IBM "Support for IBM/Lenovo notebooks" true)
	option(BUILD_HDDTEMP "Support for hddtemp" true)
	option(BUILD_WLAN "Enable wireless support" false)
	# nvidia may also work on FreeBSD, not sure
	option(BUILD_NVIDIA "Enable nvidia support" false)
	option(BUILD_IPV6 "Enable if you want IPv6 support" true)
else(OS_LINUX)
	set(BUILD_PORT_MONITORS false)
	set(BUILD_IBM false)
	set(BUILD_HDDTEMP false)
	set(BUILD_WLAN false)
	set(BUILD_NVIDIA false)
	set(BUILD_IPV6 false)
endif(OS_LINUX)

# Optional features etc
#

option(BUILD_BUILTIN_CONFIG "Enable builtin default configuration" true)

option(BUILD_IOSTATS "Enable disk I/O stats" true)

option(BUILD_OLD_CONFIG "Enable support for the old syntax of configurations" true)

option(BUILD_MATH "Enable math support" true)

option(BUILD_NCURSES "Enable ncurses support" true)
if(BUILD_NCURSES)
	option(LEAKFREE_NCURSES "Enable to hide false ncurses-memleaks in valgrind (works only when ncurses is compiled with --disable-leaks)" false)
else(BUILD_NCURSES)
	set(LEAKFREE_NCURSES false CACHE BOOL "Enable to hide false ncurses-memleaks in valgrind (works only when ncurses is compiled with --disable-leaks)" FORCE)
endif(BUILD_NCURSES)

option(BUILD_X11 "Build X11 support" true)
if(BUILD_X11)
	option(OWN_WINDOW "Enable own_window support" true)
	option(BUILD_XDAMAGE "Build Xdamage support" true)
	option(BUILD_XDBE "Build Xdbe (double-buffer) support" false)
	option(BUILD_XFT "Build Xft (freetype fonts) support" true)
	option(BUILD_IMLIB2 "Enable Imlib2 support" false)
else(BUILD_X11)
	set(OWN_WINDOW false CACHE BOOL "Enable own_window support" FORCE)
	set(BUILD_XDAMAGE false CACHE BOOL "Build Xdamage support" FORCE)
	set(BUILD_XDBE false CACHE BOOL "Build Xdbe (double-buffer) support" FORCE)
	set(BUILD_XFT false CACHE BOOL "Build Xft (freetype fonts) support" FORCE)
	set(BUILD_IMLIB2 false CACHE BOOL "Enable Imlib2 support" FORCE)
endif(BUILD_X11)

if(OWN_WINDOW)
	option(BUILD_ARGB "Build ARGB (real transparency) support" true)
else(OWN_WINDOW)
	set(BUILD_ARGB false CACHE BOOL "Build ARGB (real transparency) support" FORCE)
endif(OWN_WINDOW)

option(BUILD_LUA_CAIRO "Build cairo bindings for Lua" false)
option(BUILD_LUA_IMLIB2 "Build Imlib2 bindings for Lua" false)

option(BUILD_AUDACIOUS "Build audacious (music player) support" false)

option(BUILD_BMPX "Build BMPx (music player) support" false)

option(BUILD_MPD "Enable if you want MPD (music player) support" true)

option(BUILD_MYSQL "Enable if you want MySQL support" false)

option(BUILD_MOC "Enable if you want MOC (music player) support" true)

option(BUILD_XMMS2 "Enable if you want XMMS2 (music player) support" false)

option(BUILD_EVE "Enable if you want Eve-Online skill monitoring support" false)

option(BUILD_CURL "Enable if you want Curl support" false)

option(BUILD_RSS "Enable if you want RSS support" false)

option(BUILD_WEATHER_METAR "Enable METAR weather support" false)
option(BUILD_WEATHER_XOAP "Enable XOAP weather support" false)
if(BUILD_WEATHER_METAR OR BUILD_WEATHER_XOAP OR BUILD_RSS)
	set(BUILD_CURL true)
endif(BUILD_WEATHER_METAR OR BUILD_WEATHER_XOAP OR BUILD_RSS)
if(BUILD_WEATHER_XOAP)
	set(XOAP_FILE "$HOME/.xoaprc" CACHE STRING "Path of XOAP file for weather" FORCE)
endif(BUILD_WEATHER_XOAP)

option(BUILD_APCUPSD "Enable APCUPSD support" true)

option(BUILD_ICAL "Enable if you want iCalendar (RFC 5545) support" false)

option(BUILD_IRC "Enable if you want IRC support" false)

option(BUILD_HTTP "Enable if you want HTTP support" false)
if(BUILD_HTTP)
	set(HTTPPORT "10080" CACHE STRING "Port to use for out_to_http")
endif(BUILD_HTTP)

option(BUILD_ICONV "Enable iconv support" false)

option(BUILD_CMUS "Enable support for cmus music player" false)
