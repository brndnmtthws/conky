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

# Conky 2.x requires GCC 4.4 or newer
try_compile(GCC4_WORKS
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_MODULE_PATH}/gcc44test.cc
)
if(NOT GCC4_WORKS)
	message(FATAL_ERROR "Conky 2.x requires GCC 4.4.0 or newer")
endif(NOT GCC4_WORKS)

# Set system vars
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(OS_LINUX true)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(OS_FREEBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")

if(CMAKE_SYSTEM_NAME MATCHES "DragonFly")
    set(OS_DRAGONFLY true)
endif(CMAKE_SYSTEM_NAME MATCHES "DragonFly")

if(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
	set(OS_OPENBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")

if(CMAKE_SYSTEM_NAME MATCHES "Solaris")
	set(OS_SOLARIS true)
endif(CMAKE_SYSTEM_NAME MATCHES "Solaris")

if(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
	set(OS_NETBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")

if(NOT OS_LINUX AND NOT OS_FREEBSD AND NOT OS_OPENBSD AND NOT OS_DRAGONFLY)
	message(FATAL_ERROR "Your platform, '${CMAKE_SYSTEM_NAME}', is not currently supported.  Patches are welcome.")
endif(NOT OS_LINUX AND NOT OS_FREEBSD AND NOT OS_OPENBSD AND NOT OS_DRAGONFLY)

include(FindThreads)
find_package(Threads)

set(conky_libs ${CMAKE_THREAD_LIBS_INIT})
set(conky_includes ${CMAKE_BINARY_DIR})

add_definitions(-D_LARGEFILE64_SOURCE -D_POSIX_C_SOURCE=200809L) # Standard definitions
set(CMAKE_REQUIRED_DEFINITIONS
	"${CMAKE_REQUIRED_DEFINITIONS} -D_LARGEFILE64_SOURCE -D_POSIX_C_SOURCE=200809L")

if(OS_DRAGONFLY)
set(conky_libs ${conky_libs} -L/usr/pkg/lib)
set(conky_includes ${conky_includes} -I/usr/pkg/include)
endif(OS_DRAGONFLY)

# Do version stuff
set(VERSION_MAJOR "2")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")

find_program(APP_GAWK gawk)
if(NOT APP_GAWK)
	message(FATAL_ERROR "Unable to find program 'gawk'")
endif(NOT APP_GAWK)

find_program(APP_WC wc)
if(NOT APP_WC)
	message(FATAL_ERROR "Unable to find program 'wc'")
endif(NOT APP_WC)

find_program(APP_DATE date)
if(NOT APP_DATE)
	message(FATAL_ERROR "Unable to find program 'date'")
endif(NOT APP_DATE)

find_program(APP_UNAME uname)
if(NOT APP_UNAME)
	message(FATAL_ERROR "Unable to find program 'uname'")
endif(NOT APP_UNAME)

if(NOT RELEASE)
	find_program(APP_GIT git)
	if(NOT APP_GIT)
		message(FATAL_ERROR "Unable to find program 'git'")
	endif(NOT APP_GIT)
	mark_as_advanced(APP_GIT)
endif(NOT RELEASE)

mark_as_advanced(APP_GAWK APP_WC APP_DATE APP_UNAME)

#BUILD_DATE=$(LANG=en_US LC_ALL=en_US LOCALE=en_US date)
#BUILD_ARCH="$(uname -sr) ($(uname -m))"
execute_process(COMMAND ${APP_DATE} RESULT_VARIABLE RETVAL OUTPUT_VARIABLE
	BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${APP_UNAME} -srm RESULT_VARIABLE RETVAL
	OUTPUT_VARIABLE BUILD_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)

if(RELEASE)
	set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
else(RELEASE)
	set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}_pre${COMMIT_COUNT}")
endif(RELEASE)

set(COPYRIGHT "Copyright Brenden Matthews, et al, 2005-2010")

macro(AC_SEARCH_LIBS FUNCTION_NAME INCLUDES TARGET_VAR)
	if("${TARGET_VAR}" MATCHES "^${TARGET_VAR}$")
		unset(AC_SEARCH_LIBS_TMP CACHE)
		check_symbol_exists(${FUNCTION_NAME} ${INCLUDES} AC_SEARCH_LIBS_TMP)
		if(${AC_SEARCH_LIBS_TMP})
			set(${TARGET_VAR} "" CACHE INTERNAL "Library containing ${FUNCTION_NAME}")
		else(${AC_SEARCH_LIBS_TMP})
			foreach(LIB ${ARGN})
				unset(AC_SEARCH_LIBS_TMP CACHE)
				unset(AC_SEARCH_LIBS_FOUND CACHE)
				find_library(AC_SEARCH_LIBS_TMP ${LIB})
				check_library_exists(${LIB} ${FUNCTION_NAME} ${AC_SEARCH_LIBS_TMP}
									AC_SEARCH_LIBS_FOUND)
				if(${AC_SEARCH_LIBS_FOUND})
					set(${TARGET_VAR} ${AC_SEARCH_LIBS_TMP} CACHE INTERNAL
							"Library containing ${FUNCTION_NAME}")
					break()
				endif(${AC_SEARCH_LIBS_FOUND})
			endforeach(LIB)
		endif(${AC_SEARCH_LIBS_TMP})
	endif("${TARGET_VAR}" MATCHES "^${TARGET_VAR}$")
endmacro(AC_SEARCH_LIBS)
