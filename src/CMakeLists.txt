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

include_directories(${conky_includes})

# always include the binary dir
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)
  # include config output dir
  include_directories(${CMAKE_BINARY_DIR}/data)
endif(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)

# ensure build.h and config.h aren't in the way
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  message(
    FATAL_ERROR
    "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/config.h' in order to build with CMake."
  )
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)
  message(
    FATAL_ERROR
    "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/build.h' in order to build with CMake."
  )
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h)
  message(
    FATAL_ERROR
    "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h' in order to build with CMake."
  )
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h)

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
  ${conky_sources}
  c++wrap.cc
  c++wrap.hh
  lua/colour-settings.cc
  lua/colour-settings.hh
  content/colours.cc
  content/colours.hh
  content/combine.cc
  content/combine.h
  common.cc
  common.h
  conky.cc
  conky.h
  core.cc
  core.h
  data/hardware/cpu.cc
  data/hardware/cpu.h
  data/hardware/diskio.cc
  data/hardware/diskio.h
  data/entropy.cc
  data/entropy.h
  data/exec.cc
  data/exec.h
  data/fs.cc
  data/fs.h
  content/gradient.cc
  content/gradient.hh
  data/network/mail.cc
  data/network/mail.h
  data/misc.cc
  data/misc.h
  data/network/net_stat.cc
  data/network/net_stat.h
  content/template.cc
  content/template.h
  data/network/mboxscan.cc
  data/network/mboxscan.h
  data/network/read_tcpip.cc
  data/network/read_tcpip.h
  content/scroll.cc
  content/scroll.h
  content/specials.cc
  content/specials.h
  data/tailhead.cc
  data/tailhead.h
  content/temphelper.cc
  content/temphelper.h
  content/text_object.cc
  content/text_object.h
  data/timeinfo.cc
  data/timeinfo.h
  data/top.cc
  data/top.h
  content/algebra.cc
  content/algebra.h
  prioqueue.cc
  prioqueue.h
  data/proc.cc
  data/proc.h
  data/user.cc
  data/user.h
  lua/luamm.cc
  lua/luamm.hh
  data/data-source.cc
  data/data-source.hh
  output/display-output.cc
  output/display-output.hh
  output/display-console.cc
  output/display-console.hh
  output/display-file.cc
  output/display-file.hh
  lua/lua-config.cc
  lua/lua-config.hh
  lua/setting.cc
  lua/setting.hh
  lua/llua.cc
  lua/llua.h
  update-cb.cc
  update-cb.hh
  logging.h
  semaphore.hh
  themespresetmanager.h
  themespresetmanager.cpp
)

# Platform specific sources
if(OS_LINUX)
  set(linux_sources
    data/os/linux.cc
    data/os/linux.h
    data/users.cc
    data/users.h
    data/hardware/sony.cc
    data/hardware/sony.h
    data/hardware/i8k.cc
    data/hardware/i8k.h
  )
  set(optional_sources ${optional_sources} ${linux_sources})
endif(OS_LINUX)

if(OS_FREEBSD)
  set(freebsd_sources
    data/os/freebsd.cc
    data/os/freebsd.h
    data/hardware/bsdapm.cc
    data/hardware/bsdapm.h
  )
  set(optional_sources ${optional_sources} ${freebsd_sources})
endif(OS_FREEBSD)

if(OS_DRAGONFLY)
  set(dragonfly_sources
    data/os/dragonfly.cc
    data/os/dragonfly.h
    data/hardware/bsdapm.cc
    data/hardware/bsdapm.h
  )
  set(optional_sources ${optional_sources} ${dragonfly_sources})
endif(OS_DRAGONFLY)

if(OS_OPENBSD)
  set(openbsd_sources
    data/os/openbsd.cc
    data/os/openbsd.h
    data/hardware/bsdapm.cc
    data/hardware/bsdapm.h
    data/os/bsdcommon.cc
    data/os/bsdcommon.h
  )
  set(optional_sources ${optional_sources} ${openbsd_sources})
endif(OS_OPENBSD)

# These below are not actually supported.  No idea what their status is.
if(OS_SOLARIS)
  set(solaris_sources data/os/solaris.cc data/os/solaris.h)
  set(optional_sources ${optional_sources} ${solaris_sources})
endif(OS_SOLARIS)

if(OS_NETBSD)
  set(netbsd_sources data/os/netbsd.cc data/os/netbsd.h data/os/bsdcommon.cc data/os/bsdcommon.h)
  set(optional_sources ${optional_sources} ${netbsd_sources})
endif(OS_NETBSD)

if(OS_HAIKU)
  set(haiku_sources data/os/haiku.cc data/os/haiku.h)
  set(optional_sources ${optional_sources} ${haiku_sources})
endif(OS_HAIKU)

if(OS_DARWIN)
  set(darwin_sources data/os/darwin.mm data/os/darwin.h data/os/darwin_sip.h i18n.h)
  set(optional_sources ${optional_sources} ${darwin_sources})
endif(OS_DARWIN)

# Optional sources
if(HAVE_SOUNDCARD_H)
  set(mixer data/audio/mixer.cc data/audio/mixer.h)
  set(optional_sources ${optional_sources} ${mixer})
endif(HAVE_SOUNDCARD_H)

if(BUILD_AUDACIOUS)
  set(audacious data/audio/audacious.cc data/audio/audacious.h)
  set(optional_sources ${optional_sources} ${audacious})
endif(BUILD_AUDACIOUS)

if(BUILD_IBM)
  set(ibm data/hardware/ibm.cc data/hardware/ibm.h data/hardware/smapi.cc data/hardware/smapi.h)
  set(optional_sources ${optional_sources} ${ibm})
endif(BUILD_IBM)

if(BUILD_MPD)
  set(mpd data/audio/mpd.cc data/audio/mpd.h data/audio/libmpdclient.cc data/audio/libmpdclient.h)
  set(optional_sources ${optional_sources} ${mpd})
endif(BUILD_MPD)

if(BUILD_MYSQL)
  set(mysql data/mysql.cc data/mysql.h)
  set(optional_sources ${optional_sources} ${mysql})
endif(BUILD_MYSQL)

if(BUILD_MOC)
  set(moc data/audio/moc.cc data/audio/moc.h)
  set(optional_sources ${optional_sources} ${moc})
endif(BUILD_MOC)

if(BUILD_CMUS)
  set(cmus data/audio/cmus.cc data/audio/cmus.h)
  set(optional_sources ${optional_sources} ${cmus})
endif(BUILD_CMUS)

if(BUILD_JOURNAL)
  set(journal data/os/journal.cc data/os/journal.h)
  set(optional_sources ${optional_sources} ${journal})
endif(BUILD_JOURNAL)

if(BUILD_XMMS2)
  set(xmms2 data/audio/xmms2.cc data/audio/xmms2.h)
  set(optional_sources ${optional_sources} ${xmms2})
endif(BUILD_XMMS2)

if(BUILD_PORT_MONITORS)
  add_library(tcp-portmon 
    data/network/libtcp-portmon.cc
    data/network/libtcp-portmon.h
  )
  set(conky_libs ${conky_libs} tcp-portmon)
  set(port_monitors
    data/network/tcp-portmon.cc
    data/network/tcp-portmon.h
  )
  set(optional_sources ${optional_sources} ${port_monitors})
endif(BUILD_PORT_MONITORS)

if(BUILD_HTTP)
  set(http
    output/display-http.cc
    output/display-http.hh
  )
  set(optional_sources ${optional_sources} ${http})
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
  set(optional_sources ${optional_sources} ${x11})
endif(BUILD_X11)

if(BUILD_GUI)
  set(gui lua/fonts.cc lua/fonts.h output/gui.cc output/gui.h)
  set(optional_sources ${optional_sources} ${gui})

  if(BUILD_MOUSE_EVENTS OR BUILD_XINPUT)
    set(mouse_events mouse-events.cc mouse-events.h)
    set(optional_sources ${optional_sources} ${mouse_events})
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

  set(optional_sources ${optional_sources} ${wl_sources})
endif(BUILD_WAYLAND)

if(BUILD_HDDTEMP)
  set(hddtemp data/hardware/hddtemp.cc data/hardware/hddtemp.h)
  set(optional_sources ${optional_sources} ${hddtemp})
endif(BUILD_HDDTEMP)

if(BUILD_CURL)
  set(ccurl_thread
    data/network/ccurl_thread.cc
    data/network/ccurl_thread.h
  )
  set(optional_sources ${optional_sources} ${ccurl_thread})
endif(BUILD_CURL)

if(BUILD_RSS)
  set(rss
    data/network/rss.cc
    data/network/rss.h
    data/network/prss.cc
    data/network/prss.h
  )
  set(optional_sources ${optional_sources} ${rss})
endif(BUILD_RSS)

if(BUILD_NVIDIA)
  set(nvidia data/hardware/nvidia.cc data/hardware/nvidia.h)
  set(optional_sources ${optional_sources} ${nvidia})
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
  set(imlib2 conky-imlib2.cc conky-imlib2.h)
  set(optional_sources ${optional_sources} ${imlib2})
endif(BUILD_IMLIB2)

if(BUILD_APCUPSD)
  set(apcupsd data/hardware/apcupsd.cc data/hardware/apcupsd.h)
  set(optional_sources ${optional_sources} ${apcupsd})
endif(BUILD_APCUPSD)

if(BUILD_ICAL)
  set(ical data/ical.cc data/ical.h)
  set(optional_sources ${optional_sources} ${ical})
endif(BUILD_ICAL)

if(BUILD_IRC)
  set(irc data/network/irc.cc data/network/irc.h)
  set(optional_sources ${optional_sources} ${irc})
endif(BUILD_IRC)

if(BUILD_ICONV)
  set(iconv data/iconv_tools.cc data/iconv_tools.h)
  set(optional_sources ${optional_sources} ${iconv})
endif(BUILD_ICONV)

if(BUILD_NCURSES)
  set(ncurses_srcs
    output/nc.cc
    output/nc.h
    output/display-ncurses.cc
    output/display-ncurses.hh
  )
  set(optional_sources ${optional_sources} ${ncurses_srcs})
endif(BUILD_NCURSES)

if(BUILD_PULSEAUDIO)
  set(pulseaudio data/audio/pulseaudio.cc data/audio/pulseaudio.h)
  set(optional_sources ${optional_sources} ${pulseaudio})
endif(BUILD_PULSEAUDIO)

if(BUILD_INTEL_BACKLIGHT)
  set(intel_backlight data/hardware/intel_backlight.cc data/hardware/intel_backlight.h)
  set(optional_sources ${optional_sources} ${intel_backlight})
endif(BUILD_INTEL_BACKLIGHT)

if(BUILD_TESTING)
  # Create a library strictly for testing
  add_library(conky_core ${conky_sources} ${optional_sources})
  add_dependencies(conky_core generated_hdr_files)
  target_link_libraries(conky_core ${conky_libs})
  add_executable(conky main.cc)
  add_dependencies(conky generated_hdr_files)
  target_link_libraries(conky conky_core ${conky_libs})
  install(TARGETS conky_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
else()
  add_executable(conky main.cc ${conky_sources} ${optional_sources})
  add_dependencies(conky generated_hdr_files)
  target_link_libraries(conky ${conky_libs})
endif()

install(TARGETS conky
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR})

if(BUILD_PORT_MONITORS)
  install(TARGETS tcp-portmon
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
endif(BUILD_PORT_MONITORS)

print_target_properties(conky)
