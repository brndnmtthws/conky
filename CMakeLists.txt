# vim: ts=4 sw=4 noet ai cindent syntax=cmake

cmake_minimum_required(VERSION 2.6)

project(conky)

set(VERSION_MAJOR "2")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")

find_program(APP_GAWK gawk)
find_program(APP_GIT git)
find_program(APP_WC wc)
find_program(APP_DATE date)
find_program(APP_UNAME uname)

# The version numbers are simply derived from the date and number of commits
# since start of month
execute_process(COMMAND
	${APP_GIT} --git-dir=${CMAKE_CURRENT_SOURCE_DIR}/.git log
	--since=${VERSION_MAJOR}-${VERSION_MINOR}-01 --pretty=oneline COMMAND
	${APP_WC} -l COMMAND ${APP_GAWK} "{print $1}" RESULT_VARIABLE RETVAL
	OUTPUT_VARIABLE COMMIT_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE)
#BUILD_DATE=$(LANG=en_US LC_ALL=en_US LOCALE=en_US date)
#BUILD_ARCH="$(uname -sr) ($(uname -m))"
execute_process(COMMAND ${APP_DATE} RESULT_VARIABLE RETVAL OUTPUT_VARIABLE
	BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${APP_UNAME} -srm RESULT_VARIABLE RETVAL OUTPUT_VARIABLE
	BUILD_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-r${COMMIT_COUNT}")
set(COPYRIGHT "Copyright Brenden Matthews, et al, 2005-2009")

# This is the directory for our custom CMake modules.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules)

include(FindThreads)
find_package(Threads)

set(conky_libs ${CMAKE_THREAD_LIBS_INIT} rt c m)
set(conky_includes ${CMAKE_BINARY_DIR})

# Include CPack module for installation
include(ConkyCPackSetup)

# Handle build options
include(ConkyBuildOptions)

# Do platform checks
include(ConkyPlatformChecks)

# setup our configuration headers
configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_SOURCE_DIR}/build.h.in ${CMAKE_BINARY_DIR}/build.h)

add_subdirectory(src)
add_subdirectory(lua)
add_subdirectory(doc)

