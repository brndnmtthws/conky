#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2018 Brenden Matthews, et. al. (see AUTHORS)
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

include_directories(${conky_includes})
if(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)
  # include config output dir
  include_directories(${CMAKE_BINARY_DIR}/data)
endif(BUILD_BUILTIN_CONFIG OR BUILD_OLD_CONFIG)

# ensure build.h and config.h aren't in the way
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/config.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)
  message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/build.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h)
  message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/defconfig.h)

set(conky_sources
  ${conky_sources}
  c++wrap.cc c++wrap.hh
  colours.cc colours.h
  combine.cc combine.h
  common.cc common.h
  conky.cc conky.h
  core.cc core.h
  diskio.cc diskio.h
  entropy.cc entropy.h
  exec.cc exec.h
  fs.cc fs.h
  mail.cc mail.h
  misc.cc misc.h
  net_stat.cc net_stat.h
  template.cc template.h
  mboxscan.cc mboxscan.h
  read_tcpip.cc read_tcpip.h
  scroll.cc scroll.h
  specials.cc specials.h
  tailhead.cc tailhead.h
  temphelper.cc temphelper.h
  text_object.cc text_object.h
  timeinfo.cc timeinfo.h
  top.cc top.h
  algebra.cc algebra.h
  prioqueue.cc prioqueue.h
  proc.cc proc.h
  user.cc user.h
  luamm.cc luamm.hh
  data-source.cc data-source.hh
  lua-config.cc lua-config.hh
  setting.cc setting.hh
  llua.cc llua.h
  update-cb.cc update-cb.hh
  logging.h
  semaphore.hh
)

# Platform specific sources
if(OS_LINUX)
  set(linux
    linux.cc linux.h
    users.cc users.h
    sony.cc sony.h
    i8k.cc i8k.h
  )
  set(optional_sources ${optional_sources} ${linux})
endif(OS_LINUX)

if(OS_FREEBSD)
  set(freebsd
    freebsd.cc freebsd.h
    bsdapm.cc bsdapm.h
  )
  set(optional_sources ${optional_sources} ${freebsd})
endif(OS_FREEBSD)

if(OS_DRAGONFLY)
  set(dragonfly
    dragonfly.cc dragonfly.h
    bsdapm.cc bsdapm.h
  )
  set(optional_sources ${optional_sources} ${dragonfly})
endif(OS_DRAGONFLY)

if(OS_OPENBSD)
  set(openbsd
    openbsd.cc openbsd.h
    bsdapm.cc bsdapm.h
  )
  set(optional_sources ${optional_sources} ${openbsd})
endif(OS_OPENBSD)

# These below are not actually supported.  No idea what their status is.
if(OS_SOLARIS)
  set(solaris
    solaris.cc solaris.h
  )
  set(optional_sources ${optional_sources} ${solaris})
endif(OS_SOLARIS)

if(OS_NETBSD)
  set(netbsd
    netbsd.cc netbsd.h
  )
  set(optional_sources ${optional_sources} ${netbsd})
endif(OS_NETBSD)

if(OS_HAIKU)
  set(haiku
    haiku.cc haiku.h
  )
  set(optional_sources ${optional_sources} ${haiku})
endif(OS_HAIKU)

if(OS_DARWIN)
  set(darwin
    darwin.cc darwin.h
    darwin_sip.h
    i18n.h
  )
  set(optional_sources ${optional_sources} ${darwin})
endif(OS_DARWIN)

# Optional sources
if(HAVE_SOME_SOUNDCARD_H)
  set(mixer
    mixer.cc mixer.h
  )
  set(optional_sources ${optional_sources} ${mixer})
endif(HAVE_SOME_SOUNDCARD_H)

if(BUILD_AUDACIOUS)
  set(audacious
    audacious.cc audacious.h
  )
  set(optional_sources ${optional_sources} ${audacious})
endif(BUILD_AUDACIOUS)

if(BUILD_BMPX)
  set(bmpx
    bmpx.cc bmpx.h
  )
  set(optional_sources ${optional_sources} ${bmpx})
endif(BUILD_BMPX)

if(BUILD_IBM)
  set(ibm
    ibm.cc ibm.h
    smapi.cc smapi.h
  )
  set(optional_sources ${optional_sources} ${ibm})
endif(BUILD_IBM)

if(BUILD_MPD)
  set(mpd
    mpd.cc mpd.h
    libmpdclient.cc libmpdclient.h
  )
  set(optional_sources ${optional_sources} ${mpd})
endif(BUILD_MPD)

if(BUILD_MYSQL)
  set(mysql
    mysql.cc mysql.h
  )
  set(optional_sources ${optional_sources} ${mysql})
endif(BUILD_MYSQL)

if(BUILD_MOC)
  set(moc
    moc.cc moc.h
  )
  set(optional_sources ${optional_sources} ${moc})
endif(BUILD_MOC)

if(BUILD_CMUS)
  set(cmus
    cmus.cc cmus.h
  )
  set(optional_sources ${optional_sources} ${cmus})
endif(BUILD_CMUS)

if(BUILD_JOURNAL)
  set(journal
    journal.cc journal.h
  )
  set(optional_sources ${optional_sources} ${journal})
endif(BUILD_JOURNAL)

if(BUILD_XMMS2)
  set(xmms2
    xmms2.cc xmms2.h
  )
  set(optional_sources ${optional_sources} ${xmms2})
endif(BUILD_XMMS2)

if(BUILD_PORT_MONITORS)
  add_library(tcp-portmon libtcp-portmon.cc libtcp-portmon.h)
  set(conky_libs ${conky_libs} tcp-portmon)
  set(port_monitors
    tcp-portmon.cc tcp-portmon.h
  )
  set(optional_sources ${optional_sources} ${port_monitors})
endif(BUILD_PORT_MONITORS)

if(BUILD_X11)
  set(x11
    x11.cc x11.h
    fonts.cc fonts.h
  )
  set(optional_sources ${optional_sources} ${x11})

  if(BUILD_XINERAMA)
    find_package(Xinerama REQUIRED)
    set(conky_libs ${conky_libs} ${Xinerama_LIBRARIES})
  endif(BUILD_XINERAMA)
endif(BUILD_X11)

if(BUILD_HDDTEMP)
  set(hddtemp
    hddtemp.cc hddtemp.h
  )
  set(optional_sources ${optional_sources} ${hddtemp})
endif(BUILD_HDDTEMP)

if(BUILD_EVE)
  set(eve
    eve.cc eve.h
  )
  set(optional_sources ${optional_sources} ${eve})
endif(BUILD_EVE)

if(BUILD_CURL)
  set(ccurl_thread
    ccurl_thread.cc ccurl_thread.h
  )
  set(optional_sources ${optional_sources} ${ccurl_thread})
endif(BUILD_CURL)

if(BUILD_RSS)
  set(rss
    rss.cc rss.h
    prss.cc prss.h
  )
  set(optional_sources ${optional_sources} ${rss})
endif(BUILD_RSS)

if(BUILD_WEATHER)
  set(weather
    weather.cc weather.h
  )
  set(optional_sources ${optional_sources} ${weather})
endif(BUILD_WEATHER)

if(BUILD_NVIDIA)
  set(nvidia
    nvidia.cc nvidia.h
  )
  set(optional_sources ${optional_sources} ${nvidia})
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
  set(imlib2
    imlib2.cc imlib2.h
  )
  set(optional_sources ${optional_sources} ${imlib2})
endif(BUILD_IMLIB2)

if(BUILD_APCUPSD)
  set(apcupsd
    apcupsd.cc apcupsd.h
  )
  set(optional_sources ${optional_sources} ${apcupsd})
endif(BUILD_APCUPSD)

if(BUILD_ICAL)
  set(ical
    ical.cc ical.h
  )
  set(optional_sources ${optional_sources} ${ical})
endif(BUILD_ICAL)

if(BUILD_IRC)
  set(irc
    irc.cc irc.h
  )
  set(optional_sources ${optional_sources} ${irc})
endif(BUILD_IRC)

if(BUILD_ICONV)
  set(iconv
    iconv_tools.cc iconv_tools.h
  )
  set(optional_sources ${optional_sources} ${iconv})
endif(BUILD_ICONV)

if(BUILD_NCURSES)
  set(ncurses_srcs
    nc.cc nc.h
  )
  set(optional_sources ${optional_sources} ${ncurses_srcs})
endif(BUILD_NCURSES)

if(BUILD_PULSEAUDIO)
  set(pulseaudio
    pulseaudio.cc pulseaudio.h
  )
  set(optional_sources ${optional_sources} ${pulseaudio})
endif(BUILD_PULSEAUDIO)

add_executable(conky ${conky_sources} ${optional_sources})

target_link_libraries(conky ${conky_libs})

# Install libtcp-portmon too?
install(TARGETS
  conky
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
