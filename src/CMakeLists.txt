# vim: ts=4 sw=4 noet ai cindent syntax=cmake

include_directories(${conky_includes})

# ensure build.h and config.h aren't in the way
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
	message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/config.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)
	message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/build.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)

set(conky_sources colours.c combine.c common.c conky.cc core.cc diskio.c
	entropy.c exec.c fs.c mail.c mixer.c net_stat.c template.c timed_thread.c
	mboxscan.c read_tcp.c scroll.c specials.c tailhead.c temphelper.c
	text_object.c timeinfo.c top.c algebra.c proc.c user.c)

# Platform specific sources
if(OS_LINUX)
	set(linux linux.c users.c sony.c i8k.c)
	set(optional_sources ${optional_sources} ${linux})
endif(OS_LINUX)

if(OS_FREEBSD)
	set(freebsd freebsd.c bsdapm.c)
	set(optional_sources ${optional_sources} ${freebsd})
endif(OS_FREEBSD)

if(OS_OPENBSD)
	set(openbsd openbsd.c bsdapm.c)
	set(optional_sources ${optional_sources} ${openbsd})
endif(OS_OPENBSD)

# These below are not actually supported.  No idea what their status is.
if(OS_SOLARIS)
	set(solaris solaris.c)
	set(optional_sources ${optional_sources} ${solaris})
endif(OS_SOLARIS)

if(OS_NETBSD)
	set(netbsd netbsd.c)
	set(optional_sources ${optional_sources} ${netbsd})
endif(OS_NETBSD)


# Optional sources
if(BUILD_AUDACIOUS)
	set(audacious audacious.c)
	set(optional_sources ${optional_sources} ${audacious})
endif(BUILD_AUDACIOUS)

if(BUILD_BMPX)
	set(bmpx bmpx.c)
	set(optional_sources ${optional_sources} ${bmpx})
endif(BUILD_BMPX)

if(BUILD_IBM)
	set(ibm ibm.c smapi.c)
	set(optional_sources ${optional_sources} ${ibm})
endif(BUILD_IBM)

if(BUILD_MPD)
	set(mpd mpd.c libmpdclient.c)
	set(optional_sources ${optional_sources} ${mpd})
endif(BUILD_MPD)

if(BUILD_MOC)
	set(moc moc.c)
	set(optional_sources ${optional_sources} ${moc})
endif(BUILD_MOC)

if(BUILD_XMMS2)
	set(xmms2 xmms2.c)
	set(optional_sources ${optional_sources} ${xmms2})
endif(BUILD_XMMS2)

if(BUILD_PORT_MONITORS)
	add_library(tcp-portmon libtcp-portmon.cc)
	set(conky_libs ${conky_libs} tcp-portmon)
	set(port_monitors tcp-portmon.cc)
	set(optional_sources ${optional_sources} ${port_monitors})
endif(BUILD_PORT_MONITORS)

if(BUILD_X11)
	set(x11 x11.c fonts.c)
	set(optional_sources ${optional_sources} ${x11})
endif(BUILD_X11)

if(BUILD_HDDTEMP)
	set(hddtemp hddtemp.c)
	set(optional_sources ${optional_sources} ${hddtemp})
endif(BUILD_HDDTEMP)

if(BUILD_EVE)
	set(eve eve.c)
	set(optional_sources ${optional_sources} ${eve})
endif(BUILD_EVE)

if(BUILD_CURL)
	set(ccurl_thread ccurl_thread.c)
	set(optional_sources ${optional_sources} ${ccurl_thread})
endif(BUILD_CURL)

if(BUILD_RSS)
	set(rss rss.c prss.c)
	set(optional_sources ${optional_sources} ${rss})
endif(BUILD_RSS)

if(BUILD_WEATHER)
	set(weather weather.c)
	set(optional_sources ${optional_sources} ${weather})
endif(BUILD_WEATHER)

if(BUILD_LUA)
	set(lua llua.c)
	set(optional_sources ${optional_sources} ${lua})
endif(BUILD_LUA)

if(BUILD_NVIDIA)
	set(nvidia nvidia.c)
	set(optional_sources ${optional_sources} ${nvidia})
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
	set(imlib2 imlib2.c)
	set(optional_sources ${optional_sources} ${imlib2})
endif(BUILD_IMLIB2)

if(BUILD_APCUPSD)
	set(apcupsd apcupsd.c)
	set(optional_sources ${optional_sources} ${apcupsd})
endif(BUILD_APCUPSD)

if(BUILD_ICONV)
	set(iconv iconv_tools.c)
	set(optional_sources ${optional_sources} ${iconv})
endif(BUILD_ICONV)

add_executable(conky ${conky_sources} ${optional_sources})

target_link_libraries(conky ${conky_libs})
