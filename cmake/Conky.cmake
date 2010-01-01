# vim: ts=4 sw=4 noet ai cindent syntax=cmake

# Conky 2.x requires GCC 4.4 or newer
try_compile(GCC4_WORKS
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_MODULE_PATH}/gcc44test.cc
)
if(NOT GCC4_WORKS)
	message(WARNING "Conky 2.x maybe requires GCC 4.4.0 or newer")
endif(NOT GCC4_WORKS)

# Set system vars
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(OS_LINUX true)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(OS_FREEBSD true)
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

include(FindThreads)
find_package(Threads)

set(conky_libs ${CMAKE_THREAD_LIBS_INIT})
set(conky_includes ${CMAKE_BINARY_DIR})

add_definitions(-D_GNU_SOURCE) # Standard definitions

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

# The version numbers are simply derived from the date and number of commits
# since start of month
if(DEBUG)
	execute_process(COMMAND
		${APP_GIT} --git-dir=${CMAKE_CURRENT_SOURCE_DIR}/.git log
		--since=${VERSION_MAJOR}-${VERSION_MINOR}-01 --pretty=oneline COMMAND
		${APP_WC} -l COMMAND ${APP_GAWK} "{print $1}" RESULT_VARIABLE RETVAL
		OUTPUT_VARIABLE COMMIT_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(DEBUG)
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

