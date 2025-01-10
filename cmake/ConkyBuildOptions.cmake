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

include(DependentOption)

# This flag is for developers of conky and automated tests in conky repository.
# It should not be enabled by package maintainers as produced binary is -O0.
option(MAINTAINER_MODE "Use defaults for development environment (debug, testing, etc.)" false)
option(BUILD_TESTING "Build test binary" ${MAINTAINER_MODE})
dependent_option(RUN_TESTS
  "Run tests once the build is complete"
  SHOW_IF "BUILD_TESTING"
)
# Use gcov (requires LLVM compiler) to generate code coverage
option(CODE_COVERAGE "Enable code coverage report generation" false)

if(NOT CMAKE_BUILD_TYPE)
  if(MAINTAINER_MODE)
    message(WARNING "Default build type: Debug (because MAINTAINER_MODE is set)")
    set(
      CMAKE_BUILD_TYPE Debug
      CACHE
      STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE
    )
  else()
    message(STATUS "Default build type: RelWithDebInfo")
    set(
      CMAKE_BUILD_TYPE RelWithDebInfo
      CACHE
      STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE
    )
  endif()
endif()

set(CMAKE_COMPILE_WARNING_AS_ERROR ${MAINTAINER_MODE})

# Makes builds fully reproducible for environments (such as NixOS) that prefer
# building the binary from a clean slate. This makes build defaults and process
# avoid optimizations like compiler caching and reusing already built files.
option(REPRODUCIBLE_BUILD "Makes builds fully reproducible" OFF)

if(REPRODUCIBLE_BUILD)
  set(USE_CCACHE_DEFAULT OFF)
else()
  set(USE_CCACHE_DEFAULT ON)
endif()
mark_as_advanced(USE_CCACHE_DEFAULT)
# Instead of rebuilding objects from scratch, the compiler will reuse cached
# parts of compilation in order to speed up compilation.
option(USE_CCACHE "Sccache/ccache will be used (if installed) to speed up compilation" ${USE_CCACHE_DEFAULT})

option(CHECK_CODE_QUALITY "Check code formatting/quality with clang" ${MAINTAINER_MODE})

option(RELEASE "Build release package" false)
mark_as_advanced(RELEASE)

option(BUILD_DOCS "Build documentation" false)
option(BUILD_EXTRAS "Build extras (includes syntax files for editors)" false)

set(_SOURCE_DEFINES)
#[[.md:
A `DEPENDENT_OPTION` that also adds preprocessor definitions to the build.

```cmake
conky_option(
  <option> "<help_text>" [<initial_value>]
  [DEPENDS <conditions>]
  [FALLBACK <value>]
  [SHOW_IF <conditions>]
  [TYPE (BOOL|FILE|DIR|STRING|INT|FLOAT)]
  [WARN "<warning>"]
)
```

See documentation in `./DependentOption.cmake` for details.
]]
macro(CONKY_OPTION name)
  cmake_parse_arguments("arg_conky_option"
    "" "TYPE" ""
    ${ARGN}
  )
  dependent_option(${ARGV})
  if(NOT DEFINED arg_conky_option_TYPE OR "${arg_conky_option_TYPE}" STREQUAL "BOOL")
    string(APPEND _SOURCE_DEFINES "
    if(${name})
      source_define(${name})
    endif()")
  elseif(${arg_conky_option_TYPE} MATCHES "(FILE|DIR|STRING|INTERNAL)")
    string(APPEND _SOURCE_DEFINES "
      source_define(${name} \"\\\"\${${name}}\\\"\")
    ")
  elseif(${arg_conky_option_TYPE} MATCHES "INT|FLOAT")
    string(APPEND _SOURCE_DEFINES "
      source_define(${name} \"\${${name}}\")
    ")
  endif()
  string(FIND "${name}" "BUILD_" IS_FEATURE_FLAG)
  if(IS_FEATURE_FLAG EQUAL 0)
    string(SUBSTRING "${name}" 6 -1 FEATURE_NAME)
    list(APPEND _ENABLED_FEATURES "${FEATURE_NAME}")
    unset(FEATURE_NAME)
  endif()
  unset(IS_FEATURE_FLAG)
endmacro()

conky_option(BUILD_COLOUR_NAME_MAP
  "Include mappings of colour name -> RGB (e.g. red -> ff0000)" true
)

conky_option(BUILD_I18N
  "Enable if you want internationalization support" true
)
conky_option(LOCALE_DIR "Directory containing the locales"
  "${CMAKE_INSTALL_PREFIX}/share/locale"
  SHOW_IF "BUILD_I18N"
  TYPE FILE
)

# Some standard options
conky_option(SYSTEM_CONFIG_FILE
  "Default system-wide Conky configuration file" "/etc/conky/conky.conf"
  TYPE FILE
)

# use FORCE below to make sure this changes when CMAKE_INSTALL_PREFIX is
# modified
if(NOT LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
endif(NOT LIB_INSTALL_DIR)

set(PACKAGE_LIBRARY_DIR "${LIB_INSTALL_DIR}/conky"
  CACHE STRING "Package library path (where Lua bindings are installed"
  FORCE)
source_define(PACKAGE_LIBDIR "\"${PACKAGE_LIBRARY_DIR}\"")

set(_DEFAULTNETDEV "eno1")
if(OS_DARWIN)
  set(_DEFAULTNETDEV "en0")
endif()
conky_option(DEFAULTNETDEV "Default network device" _DEFAULTNETDEV
  TYPE STRING
)
unset(_DEFAULTNETDEV)

conky_option(XDG_CONFIG_FILE "XDG config file path" "$XDG_CONFIG_HOME/conky/conky.conf"
  TYPE FILE
)
conky_option(CONFIG_FILE "$HOME config file path" "$HOME/.conkyrc"
  TYPE FILE
)
conky_option(MAX_USER_TEXT_DEFAULT "Default maximum size of config TEXT buffer, i.e. below TEXT line." 16384
  TYPE INT
)
conky_option(DEFAULT_TEXT_BUFFER_SIZE "Default size used for temporary, static text buffers" 256
  TYPE INT
)
conky_option(MAX_NET_INTERFACES "Maximum number of network devices" 256
  TYPE INT
)

# Platform specific options Linux only
conky_option(BUILD_PORT_MONITORS "Build TCP portmon support" true
  SHOW_IF "OS_LINUX"
)
conky_option(BUILD_IBM "Support for IBM/Lenovo notebooks" true
  SHOW_IF "OS_LINUX"
)
conky_option(BUILD_HDDTEMP "Support for hddtemp" true
  SHOW_IF "OS_LINUX"
)
conky_option(BUILD_IPV6 "Enable if you want IPv6 support" true
  SHOW_IF "OS_LINUX"
)

conky_option(BUILD_NCURSES
  "Enable ncurses support" true
)
conky_option(LEAKFREE_NCURSES
  "Enable to hide false ncurses-memleaks in valgrind (works only when ncurses is compiled with --disable-leaks)"
  DEPENDS "BUILD_NCURSES"
)
conky_option(BUILD_WAYLAND
  "Build Wayland support" false
)
conky_option(BUILD_X11
  "Build X11 support" true
)
conky_option(OWN_WINDOW "Enable running conky in a dedicated window" true
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Dedicated window mode only works on X11"
)
if(NOT OS_DARWIN)
  set(NOT_DARWIN 1)
else()
  set(NOT_DARWIN 0)
endif()
conky_option(BUILD_XDAMAGE
  "Build Xdamage support" "${NOT_DARWIN}" # On MacOS these cause issues so they're disabled by default
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Xdamage support requires X11"
)
conky_option(BUILD_XFIXES "Build Xfixes support" "${NOT_DARWIN}"
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Xfixes support requires X11"
)
conky_option(BUILD_ARGB "Build ARGB (real transparency) support" true
  DEPENDS "BUILD_X11;OWN_WINDOW"
  FALLBACK false
  WARN "ARGB support requires X11 and OWN_WINDOW enabled, not needed on Wayland"
)
conky_option(BUILD_XINERAMA "Build Xinerama support" true
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Xinerama support requires X11"
)
conky_option(BUILD_XDBE "Build Xdbe (double-buffer) support" true
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Xdbe based double-buffering requires X11"
)
conky_option(BUILD_XFT "Build Xft (freetype fonts) support" true
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Xft (freetype font) support requires X11"
)
conky_option(BUILD_IMLIB2 "Enable Imlib2 support" true
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Imlib2 support requires X11"
)
conky_option(BUILD_XSHAPE "Enable Xshape support" true
  DEPENDS "BUILD_X11"
  FALLBACK false
  WARN "Xshape support requires X11"
)
conky_option(BUILD_XINPUT "Build Xinput 2 support (slow)"
  DEPENDS "BUILD_X11"
  WARN "Xinput 2 support requires X11"
)
conky_option(BUILD_MOUSE_EVENTS
  "Enable mouse event support" true
  DEPENDS "BUILD_WAYLAND OR BUILD_X11"
  FALLBACK false
  WARN "Mouse event support requires Wayland or X11 enabled"
)

conky_option(BUILD_NVIDIA
  "Enable Nvidia NvCtrl variables"
  # Should be modified to use NVML directly.
  DEPENDS "BUILD_X11"
  WARN "Nvidia NvCtrl variables require X11"
)
conky_option(BUILD_IPGFREQ
  "Enable cpu freq calculation based on Intel® Power Gadget; otherwise use constant factory value"
  SHOW_IF "OS_DARWIN"
)
conky_option(BUILD_BUILTIN_CONFIG
  "Enable builtin default configuration" true
)
conky_option(BUILD_IOSTATS
  "Enable disk I/O stats" true
)
conky_option(BUILD_OLD_CONFIG
  "Enable support for the old syntax of configurations" true
)
conky_option(BUILD_MATH
  "Enable math support" true
)
conky_option(BUILD_OPENSOUNDSYS
  "Build with Open Sound System support" true
)
conky_option(BUILD_AUDACIOUS
  "Build audacious (music player) support" false
)
conky_option(BUILD_MPD
  "Enable if you want MPD (music player) support" true
)
conky_option(BUILD_MYSQL
  "Enable if you want MySQL support" false
)
conky_option(BUILD_MOC
  "Enable if you want MOC (music player) support" true
)
conky_option(BUILD_XMMS2
  "Enable if you want XMMS2 (music player) support" false
)
conky_option(BUILD_CURL
  "Enable if you want Curl support" false
)
conky_option(BUILD_RSS "Enable if you want RSS support"
  DEPENDS "BUILD_CURL"
  WARN "RSS depends on Curl support"
)
conky_option(BUILD_APCUPSD
  "Enable APCUPSD support" true
)
conky_option(BUILD_ICAL
  "Enable if you want iCalendar (RFC 5545) support" false
)
conky_option(BUILD_IRC
  "Enable if you want IRC support" false
)
conky_option(BUILD_WLAN
  "Enable wireless support" false
)
conky_option(BUILD_HTTP
  "Enable if you want HTTP support" false
)
conky_option(HTTPPORT
  "Port to use for out_to_http" 10080
  SHOW_IF "BUILD_HTTP"
  TYPE INT
)
conky_option(BUILD_ICONV
  "Enable iconv support" false
)
conky_option(BUILD_CMUS
  "Enable support for cmus music player" true
)
conky_option(BUILD_JOURNAL
  "Enable support for reading from the systemd journal" false
)
conky_option(BUILD_PULSEAUDIO
  "Enable support for Pulseaudio's default sink and source" false
)
conky_option(BUILD_INTEL_BACKLIGHT
  "Enable support for Intel backlight" false
)

# Lua module options
set(LUA_VERSION "5.3" CACHE INTERNAL
  "Version of Lua to use for tolua++ and conky"
)
conky_option(BUILD_LUA_CAIRO
  "Build Cairo bindings for Lua"
  DEPENDS "BUILD_GUI"
  WARN "Cairo Lua bindings depend on BUILD_GUI"
)
conky_option(BUILD_LUA_CAIRO_XLIB
  "Build Cairo & Xlib interoperability for Lua" true
  DEPENDS "BUILD_X11;BUILD_LUA_CAIRO"
  FALLBACK false
  WARN "Cairo Xlib Lua bindings require Cairo and X11"
)
conky_option(BUILD_LUA_IMLIB2 "Build Imlib2 bindings for Lua"
  DEPENDS "BUILD_X11;BUILD_IMLIB2"
  WARN "Imlib2 Lua bindings require X11 and Imlib2"
)
conky_option(BUILD_LUA_RSVG "Build rsvg bindings for Lua"
  DEPENDS "BUILD_GUI"
  WARN "RSVG Lua bindings depend on BUILD_GUI"
)

run_dependency_checks()

cmake_language(EVAL CODE "${_SOURCE_DEFINES}")
#message(STATUS "${PACKAGE_NAME} defines: ${_SOURCE_DEFINES}")
unset(_SOURCE_DEFINES)

if(BUILD_X11 OR BUILD_WAYLAND)
  set(BUILD_GUI true CACHE INTERNAL "Whether any GUI support should be enabled")
  source_define(BUILD_GUI)
else()
  set(BUILD_GUI false CACHE INTERNAL "Whether any GUI support should be enabled")
endif()

# Collect and print build information for debugging

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
set(c_build_flags ${CMAKE_C_FLAGS})
set(cxx_build_flags ${CMAKE_CXX_FLAGS})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(c_build_flags "${c_build_flags} ${CMAKE_C_FLAGS_DEBUG}")
  set(cxx_build_flags "${cxx_build_flags} ${CMAKE_CXX_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  set(c_build_flags "${c_build_flags} ${CMAKE_C_FLAGS_MINSIZEREL}")
  set(cxx_build_flags "${cxx_build_flags} ${CMAKE_CXX_FLAGS_MINSIZEREL}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(c_build_flags "${c_build_flags} ${CMAKE_C_FLAGS_RELEASE}")
  set(cxx_build_flags "${cxx_build_flags} ${CMAKE_CXX_FLAGS_RELEASE}")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(c_build_flags "${c_build_flags} ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  set(cxx_build_flags "${cxx_build_flags} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()
message(VERBOSE "Global C build flags: ${c_build_flags}")
message(STATUS "Global C++ build flags: ${cxx_build_flags}")
unset(c_build_flags)
unset(cxx_build_flags)
message(STATUS "Enabled ${PACKAGE_NAME} features: ${_ENABLED_FEATURES}")
unset(_ENABLED_FEATURES)
