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
	set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Wall -W -Wextra -Wunused -pedantic -Werror" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
endif(MAINTAINER_MODE)


if(CMAKE_BUILD_TYPE MATCHES "Debug")
	set(DEBUG true)
endif(CMAKE_BUILD_TYPE MATCHES "Debug")

option(RELEASE "Build release package" false)
mark_as_advanced(RELEASE)

option(MAINTAINER_MODE "Enable maintainer mode (builds docs)" false)

# Some standard options
set(SYSTEM_CONFIG_FILE "/etc/conky/conky.conf" CACHE STRING "Default system-wide Conky configuration file")
# use FORCE below to make sure this changes when CMAKE_INSTALL_PREFIX is modified
set(PACKAGE_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib/conky" CACHE STRING "Package library path (where Lua bindings are installed" FORCE)
set(DEFAULTNETDEV "eth0" CACHE STRING "Default networkdevice")
set(CONFIG_FILE "$HOME/.conkyrc" CACHE STRING "Configfile of the user")
set(MAX_SPECIALS_DEFAULT "512" CACHE STRING "Default maximum number of special things, e.g. fonts, offsets, aligns, etc.")
set(MAX_USER_TEXT_DEFAULT "16384" CACHE STRING "Default maximum size of config TEXT buffer, i.e. below TEXT line.")
set(DEFAULT_TEXT_BUFFER_SIZE "256" CACHE STRING "Default size used for temporary, static text buffers")
set(MAX_NET_INTERFACES "16" CACHE STRING "Maximum number of network devices")


# Platform specific options
# Linux only
if(OS_LINUX)
	option(BUILD_PORT_MONITORS "Build TCP portmon support" true)
	option(BUILD_IBM "Support for IBM/Lenovo notebooks" true)
	option(BUILD_HDDTEMP "Support for hddtemp" true)
	option(BUILD_IOSTATS "Enable disk I/O stats" true)
	option(BUILD_WLAN "Enable wireless support" false)
	# nvidia may also work on FreeBSD, not sure
	option(BUILD_NVIDIA "Enable nvidia support" false)
else(OS_LINUX)
	set(BUILD_PORT_MONITORS false)
	set(BUILD_IBM false)
	set(BUILD_HDDTEMP false)
	set(BUILD_IOSTATS false)
	set(BUILD_WLAN false)
	set(BUILD_NVIDIA false)
endif(OS_LINUX)

# Optional features etc
#

option(BUILD_CONFIG_OUTPUT "Enable default config file output" true)

option(BUILD_MATH "Enable math support" true)

option(BUILD_NCURSES "Enable ncurses support" true)

option(BUILD_X11 "Build X11 support" true)
if(BUILD_X11)
	option(OWN_WINDOW "Enable own_window support" true)
	option(BUILD_XDAMAGE "Build Xdamage support" true)
	option(BUILD_XDBE "Build Xdbe (double-buffer) support" true)
	option(BUILD_XFT "Build Xft (freetype fonts) support" true)
	option(BUILD_ARGB "Build ARGB (real transparency) support" true)
	option(BUILD_IMLIB2 "Enable Imlib2 support" false)
endif(BUILD_X11)

option(BUILD_LUA "Build Lua support" true)
if(BUILD_LUA)
	option(BUILD_LUA_CAIRO "Build cairo bindings for Lua" false)
	option(BUILD_LUA_IMLIB2 "Build Imlib2 bindings for Lua" false)
endif(BUILD_LUA)

option(BUILD_AUDACIOUS "Build audacious (music player) support" false)
if(BUILD_AUDACIOUS)
	option(BUILD_AUDACIOUS_LEGACY "Use legacy audacious (music player) support" false)
else(BUILD_AUDACIOUS)
	set(BUILD_AUDACIOUS_LEGACY false)
endif(BUILD_AUDACIOUS)

option(BUILD_BMPX "Build BMPx (music player) support" false)

option(BUILD_MPD "Enable if you want MPD (music player) support" true)

option(BUILD_MOC "Enable if you want MOC (music player) support" true)

option(BUILD_XMMS2 "Enable if you want XMMS2 (music player) support" false)

option(BUILD_EVE "Enable if you want Eve-Online skill monitoring support" false)

option(BUILD_CURL "Enable if you want Curl support" false)

option(BUILD_RSS "Enable if you want RSS support" false)

option(BUILD_WEATHER_METAR "Enable METAR weather support" false)
option(BUILD_WEATHER_XOAP "Enable XOAP weather support" false)
if(BUILD_WEATHER_METAR OR BUILD_WEATHER_XOAP)
	set(BUILD_CURL true)
endif(BUILD_WEATHER_METAR OR BUILD_WEATHER_XOAP)

option(BUILD_APCUPSD "Enable APCUPSD support" true)
