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
  colour-settings.cc
  colour-settings.h
  colours.cc
  colours.h
  combine.cc
  combine.h
  common.cc
  common.h
  conky.cc
  conky.h
  core.cc
  core.h
  cpu.cc
  cpu.h
  diskio.cc
  diskio.h
  entropy.cc
  entropy.h
  exec.cc
  exec.h
  fs.cc
  fs.h
  gradient.cc
  gradient.h
  mail.cc
  mail.h
  misc.cc
  misc.h
  net_stat.cc
  net_stat.h
  template.cc
  template.h
  mboxscan.cc
  mboxscan.h
  read_tcpip.cc
  read_tcpip.h
  scroll.cc
  scroll.h
  specials.cc
  specials.h
  tailhead.cc
  tailhead.h
  temphelper.cc
  temphelper.h
  text_object.cc
  text_object.h
  timeinfo.cc
  timeinfo.h
  top.cc
  top.h
  algebra.cc
  algebra.h
  prioqueue.cc
  prioqueue.h
  proc.cc
  proc.h
  user.cc
  user.h
  luamm.cc
  luamm.hh
  data-source.cc
  data-source.hh
  display-output.cc
  display-output.hh
  display-console.cc
  display-console.hh
  display-file.cc
  display-file.hh
  lua-config.cc
  lua-config.hh
  setting.cc
  setting.hh
  llua.cc
  llua.h
  update-cb.cc
  update-cb.hh
  logging.h
  semaphore.hh
)

# Platform specific sources
if(OS_LINUX)
  set(linux linux.cc linux.h users.cc users.h sony.cc sony.h i8k.cc i8k.h)
  set(optional_sources ${optional_sources} ${linux})
endif(OS_LINUX)

if(OS_FREEBSD)
  set(freebsd freebsd.cc freebsd.h bsdapm.cc bsdapm.h)
  set(optional_sources ${optional_sources} ${freebsd})
endif(OS_FREEBSD)

if(OS_DRAGONFLY)
  set(dragonfly dragonfly.cc dragonfly.h bsdapm.cc bsdapm.h)
  set(optional_sources ${optional_sources} ${dragonfly})
endif(OS_DRAGONFLY)

if(OS_OPENBSD)
  set(openbsd openbsd.cc openbsd.h bsdapm.cc bsdapm.h)
  set(optional_sources ${optional_sources} ${openbsd})
endif(OS_OPENBSD)

# These below are not actually supported.  No idea what their status is.
if(OS_SOLARIS)
  set(solaris solaris.cc solaris.h)
  set(optional_sources ${optional_sources} ${solaris})
endif(OS_SOLARIS)

if(OS_NETBSD)
  set(netbsd netbsd.cc netbsd.h)
  set(optional_sources ${optional_sources} ${netbsd})
endif(OS_NETBSD)

if(OS_HAIKU)
  set(haiku haiku.cc haiku.h)
  set(optional_sources ${optional_sources} ${haiku})
endif(OS_HAIKU)

if(OS_DARWIN)
  set(darwin darwin.mm darwin.h darwin_sip.h i18n.h)
  set(optional_sources ${optional_sources} ${darwin})
endif(OS_DARWIN)

# Optional sources
if(HAVE_SOUNDCARD_H)
  set(mixer mixer.cc mixer.h)
  set(optional_sources ${optional_sources} ${mixer})
endif(HAVE_SOUNDCARD_H)

if(BUILD_AUDACIOUS)
  set(audacious audacious.cc audacious.h)
  set(optional_sources ${optional_sources} ${audacious})
endif(BUILD_AUDACIOUS)

if(BUILD_IBM)
  set(ibm ibm.cc ibm.h smapi.cc smapi.h)
  set(optional_sources ${optional_sources} ${ibm})
endif(BUILD_IBM)

if(BUILD_MPD)
  set(mpd mpd.cc mpd.h libmpdclient.cc libmpdclient.h)
  set(optional_sources ${optional_sources} ${mpd})
endif(BUILD_MPD)

if(BUILD_MYSQL)
  set(mysql mysql.cc mysql.h)
  set(optional_sources ${optional_sources} ${mysql})
endif(BUILD_MYSQL)

if(BUILD_MOC)
  set(moc moc.cc moc.h)
  set(optional_sources ${optional_sources} ${moc})
endif(BUILD_MOC)

if(BUILD_CMUS)
  set(cmus cmus.cc cmus.h)
  set(optional_sources ${optional_sources} ${cmus})
endif(BUILD_CMUS)

if(BUILD_JOURNAL)
  set(journal journal.cc journal.h)
  set(optional_sources ${optional_sources} ${journal})
endif(BUILD_JOURNAL)

if(BUILD_XMMS2)
  set(xmms2 xmms2.cc xmms2.h)
  set(optional_sources ${optional_sources} ${xmms2})
endif(BUILD_XMMS2)

if(BUILD_PORT_MONITORS)
  add_library(tcp-portmon libtcp-portmon.cc libtcp-portmon.h)
  set(conky_libs ${conky_libs} tcp-portmon)
  set(port_monitors tcp-portmon.cc tcp-portmon.h)
  set(optional_sources ${optional_sources} ${port_monitors})
endif(BUILD_PORT_MONITORS)

if(BUILD_HTTP)
  set(http
    display-http.cc
    display-http.hh
  )
  set(optional_sources ${optional_sources} ${http})
endif(BUILD_HTTP)

if(BUILD_X11)
  set(x11
    display-x11.cc
    display-x11.hh
    x11-settings.cc
    x11-settings.h
    x11-color.cc
    x11.cc
    x11.h
  )
  set(optional_sources ${optional_sources} ${x11})
endif(BUILD_X11)

if(BUILD_GUI)
  set(gui fonts.cc fonts.h gui.cc gui.h)
  set(optional_sources ${optional_sources} ${gui})

  if(BUILD_MOUSE_EVENTS OR BUILD_XINPUT)
    set(mouse_events mouse-events.cc mouse-events.h)
    set(optional_sources ${optional_sources} ${mouse_events})
  endif(BUILD_MOUSE_EVENTS OR BUILD_XINPUT)
endif(BUILD_GUI)

if(BUILD_WAYLAND)
  set(wl_srcs
    wl.cc
    wl.h
    display-wayland.cc
    display-wayland.hh
    xdg-shell-protocol.c
    wlr-layer-shell-protocol.c
  )
  set(optional_sources ${optional_sources} ${wl_srcs})

  # generate protocol implementations
  set(XDG_PROT_DEF "${Wayland_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
    COMMAND ${Wayland_SCANNER} client-header ${XDG_PROT_DEF} xdg-shell-client-protocol.h)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
    COMMAND ${Wayland_SCANNER} private-code ${XDG_PROT_DEF} xdg-shell-protocol.c
    DEPENDS xdg-shell-client-protocol.h)

  set(WLR_LAYER_SHELL_PROT_DEF "${CMAKE_CURRENT_SOURCE_DIR}/wlr-layer-shell-unstable-v1.xml")
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-client-protocol.h
    COMMAND ${Wayland_SCANNER} client-header ${WLR_LAYER_SHELL_PROT_DEF} wlr-layer-shell-client-protocol.h)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-protocol.c
    COMMAND ${Wayland_SCANNER} private-code ${WLR_LAYER_SHELL_PROT_DEF} wlr-layer-shell-protocol.c
    DEPENDS wlr-layer-shell-client-protocol.h)
endif(BUILD_WAYLAND)

if(BUILD_HDDTEMP)
  set(hddtemp hddtemp.cc hddtemp.h)
  set(optional_sources ${optional_sources} ${hddtemp})
endif(BUILD_HDDTEMP)

if(BUILD_CURL)
  set(ccurl_thread ccurl_thread.cc ccurl_thread.h)
  set(optional_sources ${optional_sources} ${ccurl_thread})
endif(BUILD_CURL)

if(BUILD_RSS)
  set(rss rss.cc rss.h prss.cc prss.h)
  set(optional_sources ${optional_sources} ${rss})
endif(BUILD_RSS)

if(BUILD_NVIDIA)
  set(nvidia nvidia.cc nvidia.h)
  set(optional_sources ${optional_sources} ${nvidia})
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
  set(imlib2 conky-imlib2.cc conky-imlib2.h)
  set(optional_sources ${optional_sources} ${imlib2})
endif(BUILD_IMLIB2)

if(BUILD_APCUPSD)
  set(apcupsd apcupsd.cc apcupsd.h)
  set(optional_sources ${optional_sources} ${apcupsd})
endif(BUILD_APCUPSD)

if(BUILD_ICAL)
  set(ical ical.cc ical.h)
  set(optional_sources ${optional_sources} ${ical})
endif(BUILD_ICAL)

if(BUILD_IRC)
  set(irc irc.cc irc.h)
  set(optional_sources ${optional_sources} ${irc})
endif(BUILD_IRC)

if(BUILD_ICONV)
  set(iconv iconv_tools.cc iconv_tools.h)
  set(optional_sources ${optional_sources} ${iconv})
endif(BUILD_ICONV)

if(BUILD_NCURSES)
  set(ncurses_srcs
    nc.cc
    nc.h
    display-ncurses.cc
    display-ncurses.hh
  )
  set(optional_sources ${optional_sources} ${ncurses_srcs})
endif(BUILD_NCURSES)

if(BUILD_PULSEAUDIO)
  set(pulseaudio pulseaudio.cc pulseaudio.h)
  set(optional_sources ${optional_sources} ${pulseaudio})
endif(BUILD_PULSEAUDIO)

if(BUILD_INTEL_BACKLIGHT)
  set(intel_backlight intel_backlight.cc intel_backlight.h)
  set(optional_sources ${optional_sources} ${intel_backlight})
endif(BUILD_INTEL_BACKLIGHT)

if(BUILD_TESTS)
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
