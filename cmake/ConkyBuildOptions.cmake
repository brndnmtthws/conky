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

include(CMakeDependentOption)
include(DependentOption)

if(NOT CMAKE_BUILD_TYPE)
  if(MAINTAINER_MODE)
    set(
      CMAKE_BUILD_TYPE Debug
      CACHE
      STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
  else(MAINTAINER_MODE)
    set(
      CMAKE_BUILD_TYPE RelWithDebInfo
      CACHE
      STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
  endif(MAINTAINER_MODE)
endif(NOT CMAKE_BUILD_TYPE)

# -std options for all build types
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MAINTAINER_MODE)
  set(CMAKE_COMPILE_WARNING_AS_ERROR true)
  set(BUILD_TESTS true)
endif(MAINTAINER_MODE)

# Always use libc++ when compiling w/ clang
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,Clang>:-stdlib=libc++>)
add_link_options($<$<COMPILE_LANG_AND_ID:CXX,Clang>:-stdlib=libc++>)

option(CHECK_CODE_QUALITY "Check code formatting/quality with clang" false)

option(RELEASE "Build release package" false)
mark_as_advanced(RELEASE)

option(MAINTAINER_MODE "Enable maintainer mode" false)
option(CODE_COVERAGE "Enable code coverage report generation" false)

option(BUILD_DOCS "Build documentation" false)
option(BUILD_EXTRAS "Build extras (includes syntax files for editors)" false)

option(BUILD_I18N "Enable if you want internationalization support" true)

option(BUILD_COLOUR_NAME_MAP "Include mappings of colour name -> RGB (i.e., red -> ff0000)" true)

if(BUILD_I18N)
  set(LOCALE_DIR "${CMAKE_INSTALL_PREFIX}/share/locale"
    CACHE STRING "Directory containing the locales")
endif(BUILD_I18N)

# Some standard options
set(SYSTEM_CONFIG_FILE "/etc/conky/conky.conf"
  CACHE STRING "Default system-wide Conky configuration file")

# use FORCE below to make sure this changes when CMAKE_INSTALL_PREFIX is
# modified
if(NOT LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
endif(NOT LIB_INSTALL_DIR)

set(PACKAGE_LIBRARY_DIR "${LIB_INSTALL_DIR}/conky"
  CACHE STRING "Package library path (where Lua bindings are installed"
  FORCE)
set(DEFAULTNETDEV "eno1" CACHE STRING "Default networkdevice")

# Mac only override
if(OS_DARWIN)
  set(DEFAULTNETDEV "en0" CACHE STRING "Default networkdevice" FORCE)
endif(OS_DARWIN)

set(XDG_CONFIG_FILE "$HOME/.config/conky/conky.conf"
  CACHE STRING "Configfile of the user (XDG)")
set(CONFIG_FILE "$HOME/.conkyrc" CACHE STRING "Configfile of the user")
set(MAX_USER_TEXT_DEFAULT "16384"
  CACHE STRING
  "Default maximum size of config TEXT buffer, i.e. below TEXT line.")
set(DEFAULT_TEXT_BUFFER_SIZE "256"
  CACHE STRING "Default size used for temporary, static text buffers")
set(MAX_NET_INTERFACES "256" CACHE STRING "Maximum number of network devices")

# Platform specific options Linux only
cmake_dependent_option(BUILD_PORT_MONITORS "Build TCP portmon support" true
  "OS_LINUX" false)
cmake_dependent_option(BUILD_IBM "Support for IBM/Lenovo notebooks" true
  "OS_LINUX" false)
cmake_dependent_option(BUILD_HDDTEMP "Support for hddtemp" true
  "OS_LINUX" false)
cmake_dependent_option(BUILD_IPV6 "Enable if you want IPv6 support" true
  "OS_LINUX" false)

if(OS_LINUX)
  # nvidia may also work on FreeBSD, not sure
  # NvCtrl requires X11. Should be modified to use NVML directly.
  dependent_option(BUILD_NVIDIA "Enable Nvidia NvCtrl variables" false
    "BUILD_X11" false
    "Nvidia NvCtrl variables require X11")
else()
  set(BUILD_NVIDIA false CACHE BOOL "Enable Nvidia NvCtrl variables" FORCE)
endif(OS_LINUX)

# macOS Only
cmake_dependent_option(
  BUILD_IPGFREQ
  "Enable cpu freq calculation based on Intel® Power Gadget; otherwise use constant factory value"
  false
  "OS_DARWIN" false)

# Optional features etc
option(BUILD_WLAN "Enable wireless support" false)

option(BUILD_BUILTIN_CONFIG "Enable builtin default configuration" true)

option(BUILD_IOSTATS "Enable disk I/O stats" true)

option(BUILD_OLD_CONFIG "Enable support for the old syntax of configurations"
  true)

option(BUILD_MATH "Enable math support" true)

option(BUILD_NCURSES "Enable ncurses support" true)

dependent_option(LEAKFREE_NCURSES
  "Enable to hide false ncurses-memleaks in valgrind (works only when ncurses is compiled with --disable-leaks)"
  false
  "BUILD_NCURSES" false
  "LEAKFREE_NCURSES requires ncurses")

option(BUILD_WAYLAND "Build Wayland support" false)

option(BUILD_X11 "Build X11 support" true)

dependent_option(OWN_WINDOW "Enable running conky in a dedicated window" true
  "BUILD_X11" false
  "Dedicated window mode only works on X11")

# On MacOS these cause issues so they're disabled by default
if(OS_DARWIN)
  dependent_option(BUILD_XDAMAGE "Build Xdamage support" false
    "BUILD_X11" false
    "Xdamage support requires X11")
  dependent_option(BUILD_XFIXES "Build Xfixes support" false
    "BUILD_X11" false
    "Xfixes support requires X11")
else()
  dependent_option(BUILD_XDAMAGE "Build Xdamage support" true
    "BUILD_X11" false
    "Xdamage support requires X11")
  dependent_option(BUILD_XFIXES "Build Xfixes support" true
    "BUILD_X11" false
    "Xfixes support requires X11")
endif(OS_DARWIN)

dependent_option(BUILD_ARGB "Build ARGB (real transparency) support" true
  "BUILD_X11;OWN_WINDOW" false
  "ARGB support requires X11 and OWN_WINDOW enabled, not needed on Wayland")
dependent_option(BUILD_XINERAMA "Build Xinerama support" true
  "BUILD_X11" false
  "Xinerama support requires X11")
dependent_option(BUILD_XDBE "Build Xdbe (double-buffer) support" true
  "BUILD_X11" false
  "Xdbe based double-buffering requires X11")
dependent_option(BUILD_XFT "Build Xft (freetype fonts) support" true
  "BUILD_X11" false
  "Xft (freetype font) support requires X11")
dependent_option(BUILD_IMLIB2 "Enable Imlib2 support" true
  "BUILD_X11" false
  "Imlib2 support requires X11")
dependent_option(BUILD_XSHAPE "Enable Xshape support" true
  "BUILD_X11" false
  "Xshape support requires X11")
dependent_option(BUILD_XINPUT "Build Xinput 2 support" true
  "BUILD_X11" false
  "Xinput 2 support requires X11")

# if we build with any GUI support
if(BUILD_X11)
  set(BUILD_GUI true)
endif(BUILD_X11)

if(BUILD_WAYLAND)
  set(BUILD_GUI true)
endif(BUILD_WAYLAND)

dependent_option(BUILD_MOUSE_EVENTS "Enable mouse event support" true
  "BUILD_WAYLAND OR BUILD_X11" false
  "Mouse event support requires Wayland or X11 enabled")

# Lua library options
dependent_option(BUILD_LUA_CAIRO "Build Cairo bindings for Lua" false
  "BUILD_GUI" false
  "Cairo Lua bindings depend on BUILD_GUI")
dependent_option(BUILD_LUA_CAIRO_XLIB "Build Cairo & Xlib interoperability for Lua" true
  "BUILD_X11;BUILD_LUA_CAIRO" false
  "Cairo Xlib Lua bindings require Cairo and X11")
dependent_option(BUILD_LUA_IMLIB2 "Build Imlib2 bindings for Lua" false
  "BUILD_X11;BUILD_IMLIB2" false
  "Imlib2 Lua bindings require X11 and Imlib2")
dependent_option(BUILD_LUA_RSVG "Build rsvg bindings for Lua" false
  "BUILD_GUI" false
  "RSVG Lua bindings depend on BUILD_GUI")

option(BUILD_AUDACIOUS "Build audacious (music player) support" false)

option(BUILD_MPD "Enable if you want MPD (music player) support" true)

option(BUILD_MYSQL "Enable if you want MySQL support" false)

option(BUILD_MOC "Enable if you want MOC (music player) support" true)

option(BUILD_XMMS2 "Enable if you want XMMS2 (music player) support" false)

option(BUILD_CURL "Enable if you want Curl support" false)

dependent_option(BUILD_RSS "Enable if you want RSS support" false
  "BUILD_CURL" false
  "RSS depends on Curl support")

option(BUILD_APCUPSD "Enable APCUPSD support" true)

option(BUILD_ICAL "Enable if you want iCalendar (RFC 5545) support" false)

option(BUILD_IRC "Enable if you want IRC support" false)

option(BUILD_HTTP "Enable if you want HTTP support" false)

if(NOT BUILD_HTTP)
  set(HTTPPORT "10080" CACHE STRING "Port to use for out_to_http")
else(NOT BUILD_HTTP)
  set(HTTPPORT "10080")
endif(NOT BUILD_HTTP)

option(BUILD_ICONV "Enable iconv support" false)

option(BUILD_CMUS "Enable support for cmus music player" true)

option(BUILD_JOURNAL "Enable support for reading from the systemd journal"
  false)

option(BUILD_PULSEAUDIO
  "Enable support for Pulseaudio's default sink and source" false)

option(BUILD_INTEL_BACKLIGHT
  "Enable support for Intel backlight" false)

run_dependency_checks()

message(STATUS "CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})

message(STATUS "CMAKE_C_FLAGS_DEBUG: " ${CMAKE_C_FLAGS_DEBUG})
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})

message(STATUS "CMAKE_C_FLAGS_MINSIZEREL: " ${CMAKE_C_FLAGS_MINSIZEREL})
message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL: " ${CMAKE_CXX_FLAGS_MINSIZEREL})

message(STATUS "CMAKE_C_FLAGS_RELEASE: " ${CMAKE_C_FLAGS_RELEASE})
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})

message(STATUS "CMAKE_C_FLAGS_RELWITHDEBINFO: " ${CMAKE_C_FLAGS_RELWITHDEBINFO})
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: "
  ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
