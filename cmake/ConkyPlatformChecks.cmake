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

include(FindPkgConfig)
include(CheckFunctionExists)
include(CheckIncludeFile)

# Check for some headers
check_include_files(sys/statfs.h HAVE_SYS_STATFS_H)
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/inotify.h HAVE_SYS_INOTIFY_H)
check_include_files(dirent.h HAVE_DIRENT_H)

# Check for some functions
check_function_exists(strndup HAVE_STRNDUP)

# standard path to search for includes
set(INCLUDE_SEARCH_PATH /usr/include /usr/local/include)

# Set system vars
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(OS_LINUX true)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(OS_FREEBSD true)
	set(conky_libs ${conky_libs} -lkvm -ldevstat)
endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")

if(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
	set(OS_OPENBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")

if(CMAKE_SYSTEM_NAME MATCHES "Solaris")
	set(OS_SOLARIS true)
endif(CMAKE_SYSTEM_NAME MATCHES "Solaris")

if(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
	set(OS_NETBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")

if(NOT OS_LINUX AND NOT OS_FREEBSD AND NOT OS_OPENBSD)
	message(FATAL_ERROR "Your platform, '${CMAKE_SYSTEM_NAME}', is not currently supported.  Patches are welcome.")
endif(NOT OS_LINUX AND NOT OS_FREEBSD AND NOT OS_OPENBSD)

if(BUILD_MATH)
	set(conky_libs ${conky_libs} -lm)
endif(BUILD_MATH)

if(BUILD_ICAL)
	check_include_files(libical/ical.h ICAL_H_)
	if(NOT ICAL_H_)
		message(FATAL_ERROR "Unable to find libical")
	endif(NOT ICAL_H_)
	set(conky_libs ${conky_libs} -lical)
endif(BUILD_ICAL)

if(BUILD_IRC)
	check_include_files(libircclient/libircclient.h IRC_H_)
	if(NOT IRC_H_)
		message(FATAL_ERROR "Unable to find libircclient")
	endif(NOT IRC_H_)
	set(conky_libs ${conky_libs} -lircclient)
endif(BUILD_IRC)

if(BUILD_HTTP)
	find_file(HTTP_H_ microhttpd.h)
	#I'm not using check_include_files because microhttpd.h seems to need a lot of different headers and i'm not sure which...
	if(NOT HTTP_H_)
		message(FATAL_ERROR "Unable to find libmicrohttpd")
	endif(NOT HTTP_H_)
	set(conky_libs ${conky_libs} -lmicrohttpd)
endif(BUILD_HTTP)

if(BUILD_NCURSES)
	check_include_file(ncurses.h NCURSES_H)
	find_library(NCURSES_LIB NAMES ncurses)
	if(NOT NCURSES_H OR NOT NCURSES_LIB)
		message(FATAL_ERROR "Unable to find ncurses library")
	endif(NOT NCURSES_H OR NOT NCURSES_LIB)
	set(conky_libs ${conky_libs} ${NCURSES_LIB})
endif(BUILD_NCURSES)

if(BUILD_MYSQL)
	find_path(mysql_INCLUDE_PATH mysql.h ${INCLUDE_SEARCH_PATH} /usr/include/mysql /usr/local/include/mysql)
	if(NOT mysql_INCLUDE_PATH)
		message(FATAL_ERROR "Unable to find mysql.h")
	endif(NOT mysql_INCLUDE_PATH)
	set(conky_includes ${conky_includes} ${mysql_INCLUDE_PATH})
	find_library(MYSQLCLIENT_LIB NAMES mysqlclient)
	if(NOT MYSQLCLIENT_LIB)
		message(FATAL_ERROR "Unable to find mysqlclient library")
	endif(NOT MYSQLCLIENT_LIB)
	set(conky_libs ${conky_libs} ${MYSQLCLIENT_LIB})
endif(BUILD_MYSQL)

if(BUILD_WLAN)
	check_include_file(iwlib.h IWLIB_H -D_GNU_SOURCE)
	if(NOT IWLIB_H)
		message(FATAL_ERROR "Unable to find iwlib.h")
	endif(NOT IWLIB_H)
	find_library(IWLIB_LIB NAMES iw)
	if(NOT IWLIB_LIB)
		message(FATAL_ERROR "Unable to find libiw.so")
	endif(NOT IWLIB_LIB)
	set(conky_libs ${conky_libs} ${IWLIB_LIB})
	check_function_exists(iw_sockets_open IWLIB_SOCKETS_OPEN_FUNC)
endif(BUILD_WLAN)

if(BUILD_PORT_MONITORS)
	check_function_exists(getnameinfo HAVE_GETNAMEINFO)
	if(NOT HAVE_GETNAMEINFO)
		message(FATAL_ERROR "could not find getnameinfo()")
	endif(NOT HAVE_GETNAMEINFO)
	check_include_files("netdb.h;netinet/in.h;netinet/tcp.h;sys/socket.h;arpa/inet.h" HAVE_PORTMON_HEADERS)
	if(NOT HAVE_PORTMON_HEADERS)
		message(FATAL_ERROR "missing needed network header(s) for port monitoring")
	endif(NOT HAVE_PORTMON_HEADERS)
endif(BUILD_PORT_MONITORS)


# Check for iconv
if(BUILD_ICONV)
	check_include_file(iconv.h HAVE_ICONV_H)
	find_library(ICONV_LIBRARY NAMES iconv)
	if(NOT ICONV_LIBRARY)
		# maybe iconv() is provided by libc
		set(ICONV_LIBRARY "" CACHE FILEPATH "Path to the iconv library, if iconv is not provided by libc" FORCE)
	endif(NOT ICONV_LIBRARY)
	set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
	check_function_exists(iconv ICONV_FUNC)
	if(HAVE_ICONV_H AND ICONV_FUNC)
		set(conky_includes ${conky_includes} ${ICONV_INCLUDE_DIR})
		set(conky_libs ${conky_libs} ${ICONV_LIBRARY})
	else(HAVE_ICONV_H AND ICONV_FUNC)
		message(FATAL_ERROR "Unable to find iconv library")
	endif(HAVE_ICONV_H AND ICONV_FUNC)
endif(BUILD_ICONV)


# check for Xlib
if(BUILD_X11)
	include(FindX11)
	find_package(X11)
	if(X11_FOUND)
		set(conky_includes ${conky_includes} ${X11_INCLUDE_DIR})
		set(conky_libs ${conky_libs} ${X11_LIBRARIES})

		# check for Xdamage
		if(BUILD_XDAMAGE)
			if(NOT X11_Xdamage_FOUND)
				message(FATAL_ERROR "Unable to find Xdamage library")
			endif(NOT X11_Xdamage_FOUND)
			if(NOT X11_Xfixes_FOUND)
				message(FATAL_ERROR "Unable to find Xfixes library")
			endif(NOT X11_Xfixes_FOUND)
			set(conky_libs ${conky_libs} ${X11_Xdamage_LIB} ${X11_Xfixes_LIB})
		endif(BUILD_XDAMAGE)

		# check for Xft
		if(BUILD_XFT)
			find_path(freetype_INCLUDE_PATH freetype/config/ftconfig.h ${INCLUDE_SEARCH_PATH}
				/usr/include/freetype2
				/usr/local/include/freetype2)
			if(freetype_INCLUDE_PATH)
				set(freetype_FOUND true)
				set(conky_includes ${conky_includes} ${freetype_INCLUDE_PATH})
			else(freetype_INCLUDE_PATH)
				message(FATAL_ERROR "Unable to find freetype library")
			endif(freetype_INCLUDE_PATH)
			if(NOT X11_Xft_FOUND)
				message(FATAL_ERROR "Unable to find Xft library")
			endif(NOT X11_Xft_FOUND)
			set(conky_libs ${conky_libs} ${X11_Xft_LIB})
		endif(BUILD_XFT)

		# check for Xdbe
		if(BUILD_XDBE)
			if(NOT X11_Xext_FOUND)
				message(FATAL_ERROR "Unable to find Xext library (needed for Xdbe)")
			endif(NOT X11_Xext_FOUND)
			set(conky_libs ${conky_libs} ${X11_Xext_LIB})
		endif(BUILD_XDBE)
	else(X11_FOUND)
		message(FATAL_ERROR "Unable to find X11 library")
	endif(X11_FOUND)
endif(BUILD_X11)

pkg_search_module(LUA REQUIRED lua>=5.1 lua-5.1>=5.1 lua5.1>=5.1)
set(conky_libs ${conky_libs} ${LUA_LIBRARIES})
set(conky_includes ${conky_includes} ${LUA_INCLUDE_DIRS})
if(BUILD_LUA_CAIRO)
	set(WANT_TOLUA true)
	pkg_check_modules(CAIRO REQUIRED cairo cairo-xlib)
	set(luacairo_libs ${CAIRO_LIBRARIES} ${LUA_LIBRARIES})
	set(luacairo_includes ${CAIRO_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS})
	find_program(APP_PATCH patch)
	if(NOT APP_PATCH)
		message(FATAL_ERROR "Unable to find program 'patch'")
	endif(NOT APP_PATCH)
endif(BUILD_LUA_CAIRO)
if(BUILD_LUA_IMLIB2)
	set(WANT_TOLUA true)
	pkg_check_modules(IMLIB2 imlib2)
	set(luaimlib2_libs ${IMLIB2_LIB} ${LUA_LIBRARIES})
	set(luaimlib2_includes ${IMLIB2_INCLUDE_PATH} ${LUA_INCLUDE_DIRS})
endif(BUILD_LUA_IMLIB2)

if(BUILD_AUDACIOUS)
	set(WANT_GLIB true)
	pkg_check_modules(NEW_AUDACIOUS audacious>=1.4.0)
	if(NEW_AUDACIOUS_FOUND)
		pkg_check_modules(AUDACIOUS REQUIRED audclient>=1.4.0)
	else(NEW_AUDACIOUS_FOUND)
		pkg_check_modules(AUDACIOUS REQUIRED audacious<1.4.0)
	endif(NEW_AUDACIOUS_FOUND)
	set(conky_libs ${conky_libs} ${AUDACIOUS_LIBRARIES})
	set(conky_includes ${conky_includes} ${AUDACIOUS_INCLUDE_DIRS})
endif(BUILD_AUDACIOUS)

if(BUILD_BMPX)
	pkg_check_modules(BMPX REQUIRED bmp-2.0>=0.14.0)
	set(conky_libs ${conky_libs} ${BMPX_LIBRARIES})
	set(conky_includes ${conky_includes} ${BMPX_INCLUDE_DIRS})
endif(BUILD_BMPX)

if(BUILD_XMMS2)
	pkg_check_modules(XMMS2 REQUIRED xmms2-client>=0.6)
	set(conky_libs ${conky_libs} ${XMMS2_LIBRARIES})
	set(conky_includes ${conky_includes} ${XMMS2_INCLUDE_DIRS})
endif(BUILD_XMMS2)

if(BUILD_EVE)
	set(WANT_CURL true)
	set(WANT_LIBXML2 true)
endif(BUILD_EVE)

if(BUILD_CURL)
	set(WANT_CURL true)
endif(BUILD_CURL)

if(BUILD_RSS)
	set(WANT_CURL true)
	set(WANT_LIBXML2 true)
endif(BUILD_RSS)

if(BUILD_WEATHER_METAR)
	set(WANT_CURL true)
	set(BUILD_WEATHER true)
endif(BUILD_WEATHER_METAR)

if(BUILD_WEATHER_XOAP)
	set(WANT_LIBXML2 true)
	set(WANT_CURL true)
	set(BUILD_XOAP true)
	set(BUILD_WEATHER true)
endif(BUILD_WEATHER_XOAP)

if(BUILD_NVIDIA)
	find_path(XNVCtrl_INCLUDE_PATH NVCtrl/NVCtrl.h ${INCLUDE_SEARCH_PATH})
	find_library(XNVCtrl_LIB NAMES XNVCtrl)
	if(XNVCtrl_INCLUDE_PATH AND XNVCtrl_LIB)
		set(XNVCtrl_FOUND true)
		set(conky_libs ${conky_libs} ${XNVCtrl_LIB})
		set(conky_includes ${conky_includes} ${XNVCtrl_INCLUDE_PATH})
	else(XNVCtrl_INCLUDE_PATH AND XNVCtrl_LIB)
		message(FATAL_ERROR "Unable to find XNVCtrl library")
	endif(XNVCtrl_INCLUDE_PATH AND XNVCtrl_LIB)
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
	pkg_search_module(IMLIB2 REQUIRED imlib2 Imlib2)
	set(conky_libs ${conky_libs} ${IMLIB2_LIB} ${IMLIB2_LDFLAGS})
	set(conky_includes ${conky_includes} ${IMLIB2_INCLUDE_PATH})
endif(BUILD_IMLIB2)

# Common libraries
if(WANT_GLIB)
	pkg_check_modules(GLIB REQUIRED glib-2.0)
	set(conky_libs ${conky_libs} ${GLIB_LIBRARIES})
	set(conky_includes ${conky_includes} ${GLIB_INCLUDE_DIRS})
endif(WANT_GLIB)

if(WANT_CURL)
	pkg_check_modules(CURL REQUIRED libcurl)
	set(conky_libs ${conky_libs} ${CURL_LIBRARIES})
	set(conky_includes ${conky_includes} ${CURL_INCLUDE_DIRS})
endif(WANT_CURL)

if(WANT_LIBXML2)
	pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
	set(conky_libs ${conky_libs} ${LIBXML2_LIBRARIES})
	set(conky_includes ${conky_includes} ${LIBXML2_INCLUDE_DIRS})
endif(WANT_LIBXML2)

if(WANT_TOLUA)
	find_program(APP_TOLUA NAMES tolua++ tolua++5.1 tolua++-5.1)
	if(NOT APP_TOLUA)
		message(FATAL_ERROR "Unable to find program 'tolua++'")
	endif(NOT APP_TOLUA)
	find_library(TOLUA_LIBS NAMES tolua++ tolua++5.1 tolua++-5.1)
	find_path(TOLUA_INCLUDE_PATH tolua++.h ${INCLUDE_SEARCH_PATH})
	if(TOLUA_INCLUDE_PATH AND TOLUA_LIBS)
		set(TOLUA_FOUND true)
	else(TOLUA_INCLUDE_PATH AND TOLUA_LIBS)
		message(FATAL_ERROR "Unable to find tolua++ library")
	endif(TOLUA_INCLUDE_PATH AND TOLUA_LIBS)
	mark_as_advanced(APP_TOLUA TOLUA_INCLUDE_PATH TOLUA_LIBS)
	set(conky_includes ${conky_includes} ${TOLUA_INCLUDE_PATH})
	set(conky_libs ${conky_libs} ${TOLUA_LIBS})
	set(LUA_EXTRAS true)
endif(WANT_TOLUA)

# Look for doc generation programs
if(MAINTAINER_MODE)
	# Used for doc generation
	find_program(APP_DB2X_XSLTPROC db2x_xsltproc)
	if(NOT APP_DB2X_XSLTPROC)
		message(FATAL_ERROR "Unable to find program 'db2x_xsltproc'")
	endif(NOT APP_DB2X_XSLTPROC)
	find_program(APP_DB2X_MANXML db2x_manxml)
	if(NOT APP_DB2X_MANXML)
		message(FATAL_ERROR "Unable to find program 'db2x_manxml'")
	endif(NOT APP_DB2X_MANXML)
	find_program(APP_XSLTPROC xsltproc)
	if(NOT APP_XSLTPROC)
		message(FATAL_ERROR "Unable to find program 'xsltproc'")
	endif(NOT APP_XSLTPROC)
	find_program(APP_MAN man)
	if(NOT APP_MAN)
		message(FATAL_ERROR "Unable to find program 'man'")
	endif(NOT APP_MAN)
	find_program(APP_LESS less)
	if(NOT APP_LESS)
		message(FATAL_ERROR "Unable to find program 'less'")
	endif(NOT APP_LESS)
	find_program(APP_SED sed)
	if(NOT APP_SED)
		message(FATAL_ERROR "Unable to find program 'sed'")
	endif(NOT APP_SED)
	mark_as_advanced(APP_DB2X_XSLTPROC APP_DB2X_MANXML APP_XSLTPROC APP_MAN APP_SED APP_LESS)
endif(MAINTAINER_MODE)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	set(DEBUG true)
endif(CMAKE_BUILD_TYPE MATCHES "Debug")

# The version numbers are simply derived from the date and number of commits
# since start of month
if(DEBUG)
	execute_process(COMMAND
		${APP_GIT} --git-dir=${CMAKE_CURRENT_SOURCE_DIR}/.git log
		--since=${VERSION_MAJOR}-${VERSION_MINOR}-01 --pretty=oneline COMMAND
		${APP_WC} -l COMMAND ${APP_GAWK} "{print $1}" RESULT_VARIABLE RETVAL
		OUTPUT_VARIABLE COMMIT_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(DEBUG)
