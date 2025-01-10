#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2024 Brenden Matthews, et. al. (see AUTHORS) All rights
# reserved.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details. You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include(FindPkgConfig)
include(TargetPlatformTests)

#[[.md:
Interface to store all (system) dependencies and properties in. This isn't a
separate target to be built, it's a CMake equivalent of an `#include` statement,
but avoids shuffling and updating variables around from directory to
directiory which is very error prone.
]]
add_library(conky-dependencies INTERFACE)

if(OS_FREEBSD)
  target_compile_definitions(conky-dependencies INTERFACE __BSD_VISIBLE=1)

  target_link_libraries(conky-dependencies INTERFACE -lkvm -ldevstat -linotify)
  if(BUILD_IRC)
    # IRC requires linking against ssl and crypto libraries on FreeBSD
    target_link_libraries(conky-dependencies INTERFACE -lssl -lcrypto)
  endif(BUILD_IRC)
endif()

if(OS_DRAGONFLY)
  target_link_directories(conky-dependencies INTERFACE /usr/pkg/lib)
  target_include_directories(conky-dependencies INTERFACE /usr/pkg/include)
  target_link_libraries(conky-dependencies INTERFACE -ldevstat)
endif()

if(OS_OPENBSD)
  target_link_libraries(conky-dependencies INTERFACE -lkvm)
endif()

if(OS_NETBSD)
  target_link_directories(conky-dependencies INTERFACE /usr/pkg/lib)
  target_compile_definitions(conky-dependencies INTERFACE
    _NETBSD_SOURCE 
  )
  target_link_libraries(conky-dependencies INTERFACE -lkvm)
endif()

if(OS_SOLARIS)
  target_link_directories(conky-dependencies INTERFACE /usr/local/lib)
  target_link_libraries(conky-dependencies INTERFACE -lkstat)
endif()

if(OS_HAIKU)
  target_link_libraries(conky-dependencies INTERFACE -lnetwork)
endif()

# standard path to search for includes
target_include_directories(conky-dependencies INTERFACE
  /usr/include
  /usr/local/include
)
# Include generated headers
target_include_directories(conky-dependencies INTERFACE "${CMAKE_BINARY_DIR}/include")

add_testing_target(conky-dependencies)
add_testing_target(conky-options)
set(TPT_DEFINE_CB source_define)

# Check for some headers
test_includes("sys/statfs.h"                                  HAVE_SYS_STATFS_H)
test_includes("sys/param.h"                                   HAVE_SYS_PARAM_H)
test_includes("sys/inotify.h"                                 HAVE_SYS_INOTIFY_H)
test_includes("dirent.h"                                      HAVE_DIRENT_H)
test_function("cstring" "char *strndup(const char *, size_t)" HAVE_STRNDUP)
test_symbol("unistd.h"  "pipe2"                               HAVE_PIPE2)
test_symbol("fcntl.h"   "O_CLOEXEC"                           HAVE_O_CLOEXEC)

if(OS_DARWIN OR OS_FREEBSD)
  test_symbol("sys/mount.h"  statfs64 HAVE_STATFS64)
else()
  # TODO: statvfs is Solaris and POSIX.1-2001
  # statfs is Linux and Solaris
  # sys/vfs.h includes sys/statvfs.h and is more common
  test_symbol("sys/statfs.h" statfs64 HAVE_STATFS64)
endif()

test_library(rt clock_gettime "" HAVE_CLOCK_GETTIME)
if(NOT HAVE_CLOCK_GETTIME)
  if(NOT OS_DARWIN)
    message(FATAL_ERROR "clock_gettime not found!")
  endif()
else()
  target_link_libraries(conky-dependencies INTERFACE -lrt)
endif()

if(OS_LINUX)
  test_includes("linux/sockios.h" HAVE_LINUX_SOCKIOS_H)
endif()

include(FindThreads)
find_package(Threads)
target_link_libraries(conky-dependencies INTERFACE ${CMAKE_THREAD_LIBS_INIT})

# Handle Open Sound System
if(BUILD_OPENSOUNDSYS)
  if(OS_LINUX)
    test_includes("linux/soundcard.h" HAVE_SOUNDCARD_H)
  elseif(OS_OPENBSD OR OS_NETBSD)
    test_includes("soundcard.h" HAVE_SOUNDCARD_H)
    # OpenBSD (and FreeBSD?) provide emulation layer on top of sndio.
    if(HAVE_SOUNDCARD_H)
      find_library(OSS_AUDIO_LIB
        NAMES ossaudio
        PATHS /usr/lib
        /usr/local/lib)
      target_link_libraries(conky-dependencies INTERFACE ${OSS_AUDIO_LIB})
    endif()
  else()
    test_includes("sys/soundcard.h" HAVE_SOUNDCARD_H)
  endif()
endif()

if(BUILD_I18N)
  include(FindIntl)
  find_package(Intl)

  if(NOT Intl_FOUND)
    if(OS_DARWIN)
      message(WARNING "Try running `brew install gettext` for I18N support")
      # Should be present by default everywhere else
    endif(OS_DARWIN)
    message(FATAL_ERROR "Unable to find libintl")
  endif(NOT Intl_FOUND)

  target_include_directories(conky-dependencies INTERFACE ${Intl_INCLUDE_DIRS})
  target_link_libraries(conky-dependencies INTERFACE ${Intl_LIBRARIES})
endif()

if(BUILD_NCURSES AND OS_DARWIN)
  target_link_libraries(conky-dependencies INTERFACE -lncurses)
endif()

if(BUILD_WLAN AND OS_DARWIN)
  find_library(CW CoreWLAN)
  find_library(NS Foundation)
  target_link_libraries(conky-dependencies INTERFACE ${CW})
  target_link_libraries(conky-dependencies INTERFACE ${NS})
endif()

if(OS_DARWIN AND BUILD_IPGFREQ)
  find_library(IPG IntelPowerGadget)
  target_link_libraries(conky-dependencies INTERFACE ${IPG})
endif()

if(BUILD_MATH)
  target_link_libraries(conky-dependencies INTERFACE -lm)
endif()

if(BUILD_ICAL)
  test_includes(libical/ical.h ICAL_H_)

  if(NOT ICAL_H_)
    message(FATAL_ERROR "Unable to find libical")
  endif(NOT ICAL_H_)

  target_link_libraries(conky-dependencies INTERFACE -lical)
endif(BUILD_ICAL)

if(BUILD_IRC)
  find_path(IRC_H_N libircclient.h PATHS /usr/include/libircclient)
  find_path(IRC_H_S libircclient.h PATHS /usr/include)

  if(IRC_H_N)
    target_include_directories(conky-dependencies INTERFACE ${IRC_H_N})
  endif(IRC_H_N)

  if(IRC_H_N OR IRC_H_S)
    set(IRC_H_ true)
  else()
    message(FATAL_ERROR "Unable to find libircclient")
  endif(IRC_H_N OR IRC_H_S)

  target_link_libraries(conky-dependencies INTERFACE -lircclient)
endif(BUILD_IRC)

if(BUILD_IPV6)
  find_file(IF_INET6 if_inet6 PATHS /proc/net)

  if(NOT IF_INET6)
    message(WARNING "/proc/net/if_inet6 unavailable")
  endif()
endif()

if(BUILD_HTTP)
  pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd>=0.9.25)
  target_link_libraries(conky-dependencies INTERFACE ${MICROHTTPD_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${MICROHTTPD_INCLUDE_DIRS})
endif()

if(BUILD_NCURSES)
  set(CURSES_NEED_NCURSES TRUE)

  find_path(CURSES_INCLUDE_PATH
    NAMES curses.h
    PATH_SUFFIXES ncurses
    PATHS /usr/include /usr/local/include /usr/pkg/include
    REQUIRED
  )

  find_library(CURSES_LIBRARY
    NAMES curses
    PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
    REQUIRED
  )

  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_search_module(NCURSES ncurses)
    set(CURSES_LIBRARY ${NCURSES_LDFLAGS})
  endif()

  message(VERBOSE "curses include path: ${CURSES_INCLUDE_PATH}; lib: ${CURSES_LIBRARY}")
  target_link_libraries(conky-dependencies INTERFACE ${CURSES_LIBRARY})
  target_include_directories(conky-dependencies INTERFACE ${CURSES_INCLUDE_PATH})

  if(OS_NETBSD)
    cmake_path(GET CURSES_INCLUDE_PATH PARENT_PATH CURSES_PARENT)
    target_include_directories(conky-dependencies INTERFACE ${CURSES_PARENT})
  endif(OS_NETBSD)
endif(BUILD_NCURSES)

if(BUILD_MYSQL)
  find_path(mysql_INCLUDE_PATH
    mysql.h
    ${INCLUDE_SEARCH_PATH}
    /usr/include/mysql
    /usr/local/include/mysql
    REQUIRED
  )

  target_include_directories(conky-dependencies INTERFACE ${mysql_INCLUDE_PATH})
  find_library(MYSQLCLIENT_LIB
    NAMES mysqlclient
    PATHS /usr/lib
    /usr/lib64
    /usr/lib/mysql
    /usr/lib64/mysql
    /usr/local/lib
    /usr/local/lib64
    /usr/local/lib/mysql
    /usr/local/lib64/mysql
    REQUIRED
  )

  target_link_libraries(conky-dependencies INTERFACE ${MYSQLCLIENT_LIB})
endif(BUILD_MYSQL)

if(BUILD_WLAN AND OS_LINUX)
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  test_includes(iwlib.h
    IWLIB_H
    REQUIRED
  )
  find_library(IWLIB_LIB
    NAMES iw
    REQUIRED
  )

  target_link_libraries(conky-dependencies INTERFACE ${IWLIB_LIB})
  test_function("iwlib.h" "int iw_sockets_open()" IWLIB_SOCKETS_OPEN_FUNC)
endif(BUILD_WLAN AND OS_LINUX)

if(BUILD_PORT_MONITORS)
  test_function("netdb.h;sys/socket.h"
    "int getnameinfo(struct sockaddr *, socklen_t, char *, socklen_t, char *, socklen_t, int)"
    HAVE_GETNAMEINFO
    REQUIRED
  )

  test_includes(
    "netdb.h;netinet/in.h;netinet/tcp.h;sys/socket.h;arpa/inet.h"
    HAVE_PORTMON_HEADERS
    REQUIRED
  )
endif(BUILD_PORT_MONITORS)

# Check for iconv
if(BUILD_ICONV)
  test_includes("iconv.h" HAVE_ICONV_H REQUIRED)
  find_library(ICONV_LIBRARY NAMES iconv)

  if(NOT ICONV_LIBRARY)
    # maybe iconv() is provided by libc
    set(ICONV_LIBRARY ""
      CACHE FILEPATH
      "Path to separate iconv library, if not provided by libc"
      FORCE)
  endif()

  set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
  test_function("iconv.h"
    "size_t iconv (iconv_t, char **, size_t *, char **, size_t *)"
    ICONV_FUNC
    REQUIRED
  )

  target_include_directories(conky-dependencies INTERFACE ${ICONV_INCLUDE_DIR})
  target_link_libraries(conky-dependencies INTERFACE ${ICONV_LIBRARY})
endif(BUILD_ICONV)

# check for Xlib
if(BUILD_X11)
  include(FindX11)
  find_package(X11)

  if(NOT X11_FOUND)
    message(FATAL_ERROR "Unable to find X11 library")
  endif()

  target_include_directories(conky-dependencies INTERFACE ${X11_INCLUDE_DIR})
  target_link_libraries(conky-dependencies INTERFACE ${X11_LIBRARIES})

  if(BUILD_XDAMAGE)
    if(NOT X11_Xdamage_FOUND)
      message(FATAL_ERROR "Unable to find Xdamage library")
    endif(NOT X11_Xdamage_FOUND)

    if(NOT X11_Xfixes_FOUND)
      message(FATAL_ERROR "Unable to find Xfixes library")
    endif(NOT X11_Xfixes_FOUND)

    target_link_libraries(conky-dependencies INTERFACE ${X11_Xdamage_LIB} ${X11_Xfixes_LIB})
  endif(BUILD_XDAMAGE)

  if(BUILD_XSHAPE)
    if(NOT X11_Xshape_FOUND)
      message(FATAL_ERROR "Unable to find Xshape library")
    endif(NOT X11_Xshape_FOUND)

    target_link_libraries(conky-dependencies INTERFACE ${X11_Xshape_LIB})
  endif(BUILD_XSHAPE)

  # check for Xft
  if(BUILD_XFT)
    if(FREETYPE_INCLUDE_DIR_freetype2)
      set(FREETYPE_FOUND true)
      target_include_directories(conky-dependencies INTERFACE ${FREETYPE_INCLUDE_DIR_freetype2})
    else(FREETYPE_INCLUDE_DIR_freetype2)
      message(FATAL_ERROR "Unable to find freetype library")
    endif(FREETYPE_INCLUDE_DIR_freetype2)

    if(NOT X11_Xft_FOUND)
      message(FATAL_ERROR "Unable to find Xft library")
    endif(NOT X11_Xft_FOUND)

    find_package(Fontconfig REQUIRED)

    target_link_libraries(conky-dependencies INTERFACE ${X11_Xft_LIB} ${Fontconfig_LIBRARIES})
    target_include_directories(conky-dependencies INTERFACE ${FREETYPE_INCLUDE_DIR_freetype2} ${Fontconfig_INCLUDE_DIRS})
  endif(BUILD_XFT)

  # check for Xdbe
  if(BUILD_XDBE)
    if(NOT X11_Xext_FOUND)
      message(FATAL_ERROR "Unable to find Xext library (needed for Xdbe)")
    endif(NOT X11_Xext_FOUND)

    target_link_libraries(conky-dependencies INTERFACE ${X11_Xext_LIB})
  endif(BUILD_XDBE)

  # check for Xinerama
  if(BUILD_XINERAMA)
    if(NOT X11_Xinerama_FOUND)
      message(FATAL_ERROR "Unable to find Xinerama library")
    endif(NOT X11_Xinerama_FOUND)

    target_link_libraries(conky-dependencies INTERFACE ${X11_Xinerama_LIB})
  endif(BUILD_XINERAMA)

  # check for Xfixes
  if(BUILD_XFIXES)
    if(NOT X11_Xfixes_FOUND)
      message(FATAL_ERROR "Unable to find Xfixes library")
    endif(NOT X11_Xfixes_FOUND)

    target_link_libraries(conky-dependencies INTERFACE ${X11_Xfixes_LIB})
  endif(BUILD_XFIXES)

  # check for Xinput
  if(BUILD_XINPUT)
    if(NOT X11_Xinput_FOUND)
      message(FATAL_ERROR "Unable to find Xinput library")
    endif(NOT X11_Xinput_FOUND)

    target_link_libraries(conky-dependencies INTERFACE ${X11_Xinput_LIB})
  endif(BUILD_XINPUT)

  if(X11_xcb_FOUND)
    source_define(HAVE_XCB)
    target_link_libraries(conky-dependencies INTERFACE ${X11_xcb_LIB})
    target_include_directories(conky-dependencies INTERFACE ${X11_xcb_INCLUDE_PATH})

    if(X11_xcb_errors_FOUND)
      source_define(HAVE_XCB_ERRORS)
      target_link_libraries(conky-dependencies INTERFACE ${X11_xcb_LIB} ${X11_xcb_errors_LIB})
    endif(X11_xcb_errors_FOUND)
  endif(X11_xcb_FOUND)
endif()

if(BUILD_WAYLAND)
  find_package(Wayland REQUIRED)
  target_link_libraries(conky-dependencies INTERFACE ${Wayland_CLIENT_LIBRARY})
  target_include_directories(conky-dependencies INTERFACE ${Wayland_CLIENT_INCLUDE_DIR})

  find_package(PkgConfig)

  pkg_check_modules(wayland-protocols REQUIRED QUIET wayland-protocols>=1.13)
  # System Wayland protocols
  pkg_get_variable(Wayland_PROTOCOLS_DIR wayland-protocols pkgdatadir)
  # 'wayland-scanner' executable
  pkg_get_variable(Wayland_SCANNER wayland-scanner wayland_scanner)

  if(OS_DARWIN OR OS_DRAGONFLY OR OS_FREEBSD OR OS_NETBSD OR OS_OPENBSD)
    pkg_check_modules(EPOLL REQUIRED epoll-shim)
    target_link_libraries(conky-dependencies INTERFACE ${EPOLL_LINK_LIBRARIES})
    target_include_directories(conky-dependencies INTERFACE ${EPOLL_INCLUDE_DIRS})
  endif(OS_DARWIN OR OS_DRAGONFLY OR OS_FREEBSD OR OS_NETBSD OR OS_OPENBSD)

  pkg_check_modules(CAIRO REQUIRED cairo)
  target_link_libraries(conky-dependencies INTERFACE ${CAIRO_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${CAIRO_INCLUDE_DIR})

  pkg_check_modules(PANGO REQUIRED pango)
  target_link_libraries(conky-dependencies INTERFACE ${PANGO_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${PANGO_INCLUDE_DIRS})

  pkg_check_modules(PANGOCAIRO pangocairo)
  target_link_libraries(conky-dependencies INTERFACE ${PANGOCAIRO_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${PANGOCAIRO_INCLUDE_DIRS})

  pkg_check_modules(PANGOFC pangofc)
  target_link_libraries(conky-dependencies INTERFACE ${PANGOFC_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${PANGOFC_INCLUDE_DIRS})

  pkg_check_modules(PANGOFT2 pangoft2)
  target_link_libraries(conky-dependencies INTERFACE ${PANGOFT2_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${PANGOFT2_INCLUDE_DIRS})
endif()

find_package(Lua ${LUA_VERSION} REQUIRED)
target_link_libraries(conky-dependencies INTERFACE ${LUA_LIBRARIES})
target_include_directories(conky-dependencies INTERFACE ${LUA_INCLUDE_DIR})
target_include_directories(conky-dependencies INTERFACE 3rdparty/toluapp/include)

# Check for libraries used by Lua bindings
if(BUILD_LUA_CAIRO)
  pkg_check_modules(CAIRO REQUIRED cairo>=1.14)
  set(luacairo_libs ${CAIRO_LINK_LIBRARIES} ${LUA_LIBRARIES})
  set(luacairo_includes ${CAIRO_INCLUDE_DIRS} ${LUA_INCLUDE_DIR})

  if(BUILD_LUA_CAIRO_XLIB)
    pkg_check_modules(CAIROXLIB REQUIRED cairo-xlib)
    set(luacairo_libs ${CAIROXLIB_LINK_LIBRARIES} ${luacairo_libs})
    set(luacairo_includes ${CAIROXLIB_INCLUDE_DIRS} ${luacairo_includes})
  endif()

  find_program(APP_PATCH patch REQUIRED)
endif()

if(BUILD_LUA_IMLIB2)
  pkg_search_module(IMLIB2 REQUIRED imlib2 Imlib2)
  set(luaimlib2_libs ${IMLIB2_LIBS} ${IMLIB2_LDFLAGS} ${LUA_LIBRARIES})
  set(luaimlib2_includes
    ${IMLIB2_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
    ${X11_INCLUDE_DIR})
endif()

if(BUILD_LUA_RSVG)
  pkg_check_modules(RSVG REQUIRED librsvg-2.0>=2.52)
  set(luarsvg_libs ${RSVG_LINK_LIBRARIES} ${LUA_LIBRARIES})
  set(luarsvg_includes ${RSVG_INCLUDE_DIRS} ${LUA_INCLUDE_DIR})
endif()

if(BUILD_AUDACIOUS)
  set(WANT_GLIB true)
  pkg_check_modules(NEW_AUDACIOUS audacious>=1.4.0)

  if(NEW_AUDACIOUS_FOUND)
    source_define(NEW_AUDACIOUS_FOUND)
    pkg_check_modules(AUDACIOUS REQUIRED audclient>=1.4.0)
    pkg_check_modules(DBUS_GLIB REQUIRED dbus-glib-1)
  else()
    pkg_check_modules(AUDACIOUS REQUIRED audacious<1.4.0)
  endif()

  target_link_libraries(conky-dependencies INTERFACE ${AUDACIOUS_LINK_LIBRARIES} ${DBUS_GLIB_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE
    ${AUDACIOUS_INCLUDE_DIRS}
    ${DBUS_GLIB_INCLUDE_DIRS}
  )
endif()

if(BUILD_XMMS2)
  pkg_check_modules(XMMS2 REQUIRED xmms2-client>=0.6)
  target_link_libraries(conky-dependencies INTERFACE ${XMMS2_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${XMMS2_INCLUDE_DIRS})
endif()

if(BUILD_CURL)
  set(WANT_CURL true)
endif()

if(BUILD_RSS)
  set(WANT_CURL true)
  set(WANT_LIBXML2 true)
endif()

if(BUILD_NVIDIA)
  find_path(XNVCtrl_INCLUDE_PATH
    NVCtrl/NVCtrl.h
    PATHS ${INCLUDE_SEARCH_PATH}
    REQUIRED
  )
  find_library(XNVCtrl_LIB
    NAMES XNVCtrl
    REQUIRED
  )

  target_link_libraries(conky-dependencies INTERFACE ${XNVCtrl_LIB})
  target_include_directories(conky-dependencies INTERFACE ${XNVCtrl_INCLUDE_PATH})
endif()

if(BUILD_IMLIB2)
  pkg_search_module(IMLIB2 REQUIRED imlib2 Imlib2)
  target_link_libraries(conky-dependencies INTERFACE ${IMLIB2_LIBS} ${IMLIB2_LDFLAGS})
  target_include_directories(conky-dependencies INTERFACE ${IMLIB2_INCLUDE_DIRS})
endif()

if(BUILD_JOURNAL)
  pkg_search_module(SYSTEMD REQUIRED libsystemd>=205 libsystemd-journal>=205)
  target_link_libraries(conky-dependencies INTERFACE ${SYSTEMD_LIB} ${SYSTEMD_LDFLAGS})
  target_include_directories(conky-dependencies INTERFACE ${SYSTEMD_INCLUDE_DIRS})
endif()

if(BUILD_PULSEAUDIO)
  pkg_check_modules(PULSEAUDIO REQUIRED libpulse)
  target_link_libraries(conky-dependencies INTERFACE ${PULSEAUDIO_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${PULSEAUDIO_INCLUDE_DIRS})
endif()

if(WANT_CURL)
  pkg_check_modules(CURL REQUIRED libcurl)
  target_link_libraries(conky-dependencies INTERFACE ${CURL_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${CURL_INCLUDE_DIRS})
endif()

# Common libraries
if(WANT_GLIB)
  pkg_check_modules(GLIB REQUIRED glib-2.0>=2.36)
  target_link_libraries(conky-dependencies INTERFACE ${GLIB_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${GLIB_INCLUDE_DIRS})
endif()

if(WANT_CURL)
  pkg_check_modules(CURL REQUIRED libcurl)
  target_link_libraries(conky-dependencies INTERFACE ${CURL_LINK_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${CURL_INCLUDE_DIRS})
endif()

if(WANT_LIBXML2)
  include(FindLibXml2)

  if(NOT LIBXML2_FOUND)
    message(FATAL_ERROR "Unable to find libxml2 library")
  endif()

  target_link_libraries(conky-dependencies INTERFACE ${LIBXML2_LIBRARIES})
  target_include_directories(conky-dependencies INTERFACE ${LIBXML2_INCLUDE_DIR})
endif(WANT_LIBXML2)

# Look for doc generation programs
if(BUILD_DOCS)
  # Used for doc generation
  find_program(APP_PANDOC pandoc)

  if(NOT APP_PANDOC)
    message(FATAL_ERROR "Unable to find program 'pandoc'")
  endif(NOT APP_PANDOC)

  mark_as_advanced(APP_PANDOC)
endif()

if(BUILD_DOCS OR BUILD_EXTRAS)
  # Python3 with Jinja2 and PyYaml required for manpage generation.
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import yaml"
    RESULT_VARIABLE EXIT_CODE
    OUTPUT_QUIET
  )

  if(NOT ${EXIT_CODE} EQUAL 0)
    message(
      FATAL_ERROR
      "The \"PyYAML\" Python3 package is not installed. Please install it using the following command: \"pip3 install pyyaml\"."
    )
  endif()

  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import jinja2"
    RESULT_VARIABLE EXIT_CODE
    OUTPUT_QUIET
  )

  if(NOT ${EXIT_CODE} EQUAL 0)
    message(
      FATAL_ERROR
      "The \"Jinja2\" Python3 package is not installed. Please install it using the following command: \"pip3 install Jinja2\"."
    )
  endif()
endif()

if(BUILD_COLOUR_NAME_MAP)
  find_program(APP_GPERF gperf REQUIRED) # required at build-time as of Conky v1.20.2
  mark_as_advanced(APP_GPERF)
endif(BUILD_COLOUR_NAME_MAP)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(DEBUG true)
endif()

# The version numbers are simply derived from the date and number of commits
# since start of month
if(DEBUG)
  execute_process(COMMAND ${APP_GIT} --git-dir=${CMAKE_CURRENT_SOURCE_DIR}/.git
    log --since=${VERSION_MAJOR}-${VERSION_MINOR}-01
    --pretty=oneline
    COMMAND ${APP_WC} -l
    COMMAND ${APP_AWK} "{print $1}"
    RESULT_VARIABLE RETVAL
    OUTPUT_VARIABLE COMMIT_COUNT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

clear_testing_targets()
