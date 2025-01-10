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

# Generate colour-names.hh with gperf
execute_process(
  INPUT_FILE "${CMAKE_SOURCE_DIR}/data/color-names.yml"
  OUTPUT_FILE "${CMAKE_BINARY_DIR}/data/color-names.gperf"
  COMMAND sh "${CMAKE_SOURCE_DIR}/bin/format-colors.sh"
)
execute_process(
  INPUT_FILE "${CMAKE_BINARY_DIR}/data/color-names.gperf"
  OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/colour-names.hh"
  COMMAND ${APP_GPERF} --ignore-case -LC++ -Zcolor_name_hash -t -7 -m1 -C -E
)

set(conky_sources
  c++wrap.cc
  c++wrap.hh
  common.cc
  common.h
  conky.cc
  conky.h
  content/algebra.cc
  content/algebra.h
  content/colours.cc
  content/colours.hh
  content/combine.cc
  content/combine.h
  content/gradient.cc
  content/gradient.hh
  content/scroll.cc
  content/scroll.h
  content/specials.cc
  content/specials.h
  content/temphelper.cc
  content/temphelper.h
  content/template.cc
  content/template.h
  content/text_object.cc
  content/text_object.h
  core.cc
  core.h
  data/data-source.cc
  data/data-source.hh
  data/entropy.cc
  data/entropy.h
  data/exec.cc
  data/exec.h
  data/fs.cc
  data/fs.h
  data/hardware/cpu.cc
  data/hardware/cpu.h
  data/hardware/diskio.cc
  data/hardware/diskio.h
  data/misc.cc
  data/misc.h
  data/network/mail.cc
  data/network/mail.h
  data/network/mboxscan.cc
  data/network/mboxscan.h
  data/network/net_stat.cc
  data/network/net_stat.h
  data/network/read_tcpip.cc
  data/network/read_tcpip.h
  data/proc.cc
  data/proc.h
  data/tailhead.cc
  data/tailhead.h
  data/timeinfo.cc
  data/timeinfo.h
  data/top.cc
  data/top.h
  data/user.cc
  data/user.h
  i18n.h
  logging.h
  lua/colour-settings.cc
  lua/colour-settings.hh
  lua/llua.cc
  lua/llua.h
  lua/lua-config.cc
  lua/lua-config.hh
  lua/luamm.cc
  lua/luamm.hh
  lua/setting.cc
  lua/setting.hh
  output/display-console.cc
  output/display-console.hh
  output/display-file.cc
  output/display-file.hh
  output/display-output.cc
  output/display-output.hh
  prioqueue.cc
  prioqueue.h
  semaphore.hh
  update-cb.cc
  update-cb.hh
)
add_library(conky-core SHARED ${conky_sources})
add_dependencies(conky-core generated_hdr_files)
target_include_directories(conky-core PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(conky-core
  PRIVATE conky-options
  PUBLIC conky-definitions conky-dependencies
)
set_target_properties(conky-core PROPERTIES
  COMPILE_FLAGS -fPIC # required to make lua modules interact with conky binary
  OUTPUT_NAME conky
)

if(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)
  # include config output dir
  # NOTE: this is `<root>/data`, not `<root>/src/data`
  target_include_directories(conky-core PUBLIC "${CMAKE_BINARY_DIR}/data")
endif(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)

if(BUILD_TESTING)
  set(PUBLIC_TESTING PUBLIC)
else()
  set(PUBLIC_TESTING PRIVATE)
endif()

# Platform specific sources
if(OS_LINUX)
  target_sources(conky-core ${PUBLIC_TESTING} 
    data/os/linux.cc
    data/os/linux.h
    data/users.cc
    data/users.h
    data/hardware/sony.cc
    data/hardware/sony.h
    data/hardware/i8k.cc
    data/hardware/i8k.h
  )
elseif(OS_FREEBSD)
  target_sources(conky-core ${PUBLIC_TESTING} 
    data/os/freebsd.cc
    data/os/freebsd.h
    data/hardware/bsdapm.cc
    data/hardware/bsdapm.h
  )
elseif(OS_DRAGONFLY)
  target_sources(conky-core ${PUBLIC_TESTING}
    data/os/dragonfly.cc
    data/os/dragonfly.h
    data/hardware/bsdapm.cc
    data/hardware/bsdapm.h
  )
elseif(OS_OPENBSD)
  target_sources(conky-core ${PUBLIC_TESTING} 
    data/os/openbsd.cc
    data/os/openbsd.h
    data/hardware/bsdapm.cc
    data/hardware/bsdapm.h
  )
elseif(OS_SOLARIS)
  target_sources(conky-core ${PUBLIC_TESTING}
    data/os/solaris.cc
    data/os/solaris.h
  )
elseif(OS_NETBSD)
  target_sources(conky-core ${PUBLIC_TESTING}
    data/os/netbsd.cc
    data/os/netbsd.h
    data/os/bsdcommon.cc
    data/os/bsdcommon.h
  )
elseif(OS_HAIKU)
  set(haiku_sources )
  target_sources(conky-core ${PUBLIC_TESTING}
    data/os/haiku.cc
    data/os/haiku.h
  )
elseif(OS_DARWIN)
  set(darwin_sources )
  target_sources(conky-core ${PUBLIC_TESTING}
    data/os/darwin.mm
    data/os/darwin.h
    data/os/darwin_sip.h
  )
endif()

# Optional sources
if(HAVE_SOUNDCARD_H)
  target_sources(conky-core PUBLIC
    data/audio/mixer.cc
    data/audio/mixer.h
  )
endif(HAVE_SOUNDCARD_H)

if(BUILD_AUDACIOUS)
  target_sources(conky-core PUBLIC
    data/audio/audacious.cc
    data/audio/audacious.h
  )
endif(BUILD_AUDACIOUS)

if(BUILD_IBM)
  target_sources(conky-core PUBLIC
    data/hardware/ibm.cc
    data/hardware/ibm.h
    data/hardware/smapi.cc
    data/hardware/smapi.h
  )
endif(BUILD_IBM)

if(BUILD_MPD)
  target_sources(conky-core PUBLIC
    data/audio/mpd.cc
    data/audio/mpd.h
    data/audio/libmpdclient.cc
    data/audio/libmpdclient.h
  )
endif(BUILD_MPD)

if(BUILD_MYSQL)
  target_sources(conky-core PUBLIC
    data/mysql.cc
    data/mysql.h
  )
endif(BUILD_MYSQL)

if(BUILD_MOC)
  target_sources(conky-core PUBLIC
    data/audio/moc.cc
    data/audio/moc.h
  )
endif(BUILD_MOC)

if(BUILD_CMUS)
  target_sources(conky-core PUBLIC
    data/audio/cmus.cc
    data/audio/cmus.h
  )
endif(BUILD_CMUS)

if(BUILD_JOURNAL)
  target_sources(conky-core PUBLIC 
    data/os/journal.cc
    data/os/journal.h
  )
endif(BUILD_JOURNAL)

if(BUILD_XMMS2)
  target_sources(conky-core PUBLIC
    data/audio/xmms2.cc
    data/audio/xmms2.h
  )
endif(BUILD_XMMS2)

if(BUILD_PORT_MONITORS)
  add_library(tcp-portmon 
    data/network/libtcp-portmon.cc
    data/network/libtcp-portmon.h
  )
  set_target_properties(tcp-portmon PROPERTIES
    COMPILE_FLAGS -fPIC # conky-core is, so this must be as well
  )
  target_link_libraries(conky-core PUBLIC tcp-portmon)
  target_sources(conky-core PUBLIC
    data/network/tcp-portmon.cc
    data/network/tcp-portmon.h
  )
endif(BUILD_PORT_MONITORS)

if(BUILD_HTTP)
  set(http
    output/display-http.cc
    output/display-http.hh
  )
  target_sources(conky-core PUBLIC ${http})
endif(BUILD_HTTP)

if(BUILD_X11)
  set(x11
    output/display-x11.cc
    output/display-x11.hh
    lua/x11-settings.cc
    lua/x11-settings.h
    output/x11-color.cc
    output/x11.cc
    output/x11.h
  )
  target_sources(conky-core PUBLIC ${x11})
endif(BUILD_X11)

if(BUILD_GUI)
  target_sources(conky-core PUBLIC
    lua/fonts.cc
    lua/fonts.h
    output/gui.cc
    output/gui.h
  )

  if(BUILD_MOUSE_EVENTS OR BUILD_XINPUT)
    target_sources(conky-core PUBLIC
      mouse-events.cc
      mouse-events.h
    )
  endif(BUILD_MOUSE_EVENTS OR BUILD_XINPUT)
endif(BUILD_GUI)

if(BUILD_WAYLAND)
  set(wl_sources
    output/wl.cc
    output/wl.h
    output/display-wayland.cc
    output/display-wayland.hh
    xdg-shell-protocol.c
    wlr-layer-shell-protocol.c
  )

  # Looks up wayland protocol files, build respective headers/sources, and adds
  # them to the build.
  #
  # Use: add_protocol(name [v<version>])
  macro(ADD_PROTOCOL name)
    set(WL_PROTOCOL_PATHS
      "${CMAKE_CURRENT_SOURCE_DIR}/wl_protocols" # first for reproducibility
      "${Wayland_PROTOCOLS_DIR}/stable/${name}"
      "${Wayland_PROTOCOLS_DIR}/stable"
      "${Wayland_PROTOCOLS_DIR}/unstable/${name}"
      "${Wayland_PROTOCOLS_DIR}/unstable"
    )
    if(${ARGC} GREATER 1)
      set(VERSION ${ARGV1})
      find_file(PROTOCOL_FILE
        NAMES
          "${name}-${VERSION}.xml"
          "${name}-unstable-${VERSION}.xml"
        PATHS ${WL_PROTOCOL_PATHS}
        NO_CACHE
        REQUIRED
        NO_DEFAULT_PATH
      )
      message(STATUS "PROTOCOL '${name}' ${VERSION} file: ${PROTOCOL_FILE}")
      unset(VERSION)
    else()
      find_file(PROTOCOL_FILE
        NAMES
          "${name}.xml"
          "${name}-unstable.xml"
        PATHS ${WL_PROTOCOL_PATHS}
        NO_CACHE
        REQUIRED
        NO_DEFAULT_PATH
      )
      message(STATUS "PROTOCOL '${name}' file: ${PROTOCOL_FILE}")
    endif()
    
    
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}-client-protocol.h"
      COMMAND ${Wayland_SCANNER} client-header "${PROTOCOL_FILE}" "${name}-client-protocol.h")
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}-protocol.c
      COMMAND ${Wayland_SCANNER} private-code "${PROTOCOL_FILE}" "${name}-protocol.c"
      DEPENDS "${name}-client-protocol.h")
    list(APPEND wl_sources "${name}-protocol.c")
    unset(PROTOCOL_FILE)
    unset(WL_PROTOCOL_PATHS)
  endmacro()
  
  add_protocol(xdg-shell)
  add_protocol(wlr-layer-shell v1)

  target_sources(conky-core PUBLIC ${wl_sources})
endif(BUILD_WAYLAND)

if(BUILD_HDDTEMP)
  set(hddtemp data/hardware/hddtemp.cc data/hardware/hddtemp.h)
  target_sources(conky-core PUBLIC ${hddtemp})
endif(BUILD_HDDTEMP)

if(BUILD_CURL)
  set(ccurl_thread
    data/network/ccurl_thread.cc
    data/network/ccurl_thread.h
  )
  target_sources(conky-core PUBLIC ${ccurl_thread})
endif(BUILD_CURL)

if(BUILD_RSS)
  target_sources(conky-core PUBLIC
    data/network/rss.cc
    data/network/rss.h
    data/network/prss.cc
    data/network/prss.h
  )
endif(BUILD_RSS)

if(BUILD_NVIDIA)
  target_sources(conky-core PUBLIC
    data/hardware/nvidia.cc
    data/hardware/nvidia.h
  )
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
  target_sources(conky-core PUBLIC
    conky-imlib2.cc
    conky-imlib2.h
  )
endif(BUILD_IMLIB2)

if(BUILD_APCUPSD)
  target_sources(conky-core PUBLIC
    data/hardware/apcupsd.cc
    data/hardware/apcupsd.h
  )
endif(BUILD_APCUPSD)

if(BUILD_ICAL)
  target_sources(conky-core PUBLIC
    data/ical.cc
    data/ical.h
  )
endif(BUILD_ICAL)

if(BUILD_IRC)
  target_sources(conky-core PUBLIC
    data/network/irc.cc
    data/network/irc.h
  )
endif(BUILD_IRC)

if(BUILD_ICONV)
  target_sources(conky-core PUBLIC
    data/iconv_tools.cc
    data/iconv_tools.h
  )
endif(BUILD_ICONV)

if(BUILD_NCURSES)
  target_sources(conky-core PUBLIC
    output/nc.cc
    output/nc.h
    output/display-ncurses.cc
    output/display-ncurses.hh
  )
endif(BUILD_NCURSES)

if(BUILD_PULSEAUDIO)
  target_sources(conky-core PUBLIC
    data/audio/pulseaudio.cc
    data/audio/pulseaudio.h
  )
endif(BUILD_PULSEAUDIO)

if(BUILD_INTEL_BACKLIGHT)
  target_sources(conky-core PUBLIC
    data/hardware/intel_backlight.cc
    data/hardware/intel_backlight.h
  )
endif(BUILD_INTEL_BACKLIGHT)

install(TARGETS conky-core
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
print_target_properties(conky-core)

if(BUILD_PORT_MONITORS)
  install(TARGETS tcp-portmon
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
endif()
