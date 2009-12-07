# vim: ts=4 sw=4 noet ai cindent syntax=cmake

include_directories(${conky_includes})

# ensure build.h and config.h aren't in the way
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
	message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/config.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)
	message(FATAL_ERROR "You must remove the autotools generated file '${CMAKE_CURRENT_SOURCE_DIR}/build.h' in order to build with CMake.")
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build.h)

set(conky_sources colours.c combine.c common.c conky.c core.c diskio.c
	entropy.c exec.c fs.c mail.c mixer.c net_stat.c template.c timed_thread.c
	mboxscan.c read_tcp.c scroll.c specials.c tailhead.c temphelper.c
	text_object.c timeinfo.c top.c algebra.c proc.c user.c)

# Platform specific sources
if(OS_LINUX)
	set(linux linux.c linux.h users.c users.h sony.c sony.h i8k.c i8k.h)
	set(optional_sources ${optional_sources} ${linux})
endif(OS_LINUX)

if(OS_FREEBSD)
	set(freebsd freebsd.c freebsd.h bsdapm.c bsdapm.h)
	set(optional_sources ${optional_sources} ${freebsd})
endif(OS_FREEBSD)

if(OS_OPENBSD)
	set(openbsd openbsd.c openbsd.h bsdapm.c bsdapm.h)
	set(optional_sources ${optional_sources} ${openbsd})
endif(OS_OPENBSD)

# These below are not actually supported.  No idea what their status is.
if(OS_SOLARIS)
	set(solaris solaris.c)
	set(optional_sources ${optional_sources} ${solaris})
endif(OS_SOLARIS)

if(OS_NETBSD)
	set(netbsd netbsd.c netbsd.h)
	set(optional_sources ${optional_sources} ${netbsd})
endif(OS_NETBSD)


# Optional sources
if(BUILD_AUDACIOUS)
	set(audacious audacious.c audacious.h)
	set(optional_sources ${optional_sources} ${audacious})
endif(BUILD_AUDACIOUS)

if(BUILD_BMPX)
	set(bmpx bmpx.c bmpx.h)
	set(optional_sources ${optional_sources} ${bmpx})
endif(BUILD_BMPX)

if(BUILD_IBM)
	set(ibm ibm.c ibm.h smapi.c smapi.h)
	set(optional_sources ${optional_sources} ${ibm})
endif(BUILD_IBM)

if(BUILD_MPD)
	set(mpd mpd.c mpd.h libmpdclient.c libmpdclient.h)
	set(optional_sources ${optional_sources} ${mpd})
endif(BUILD_MPD)

if(BUILD_MOC)
	set(moc moc.c moc.h)
	set(optional_sources ${optional_sources} ${moc})
endif(BUILD_MOC)

if(BUILD_XMMS2)
	set(xmms2 xmms2.c xmms2.h)
	set(optional_sources ${optional_sources} ${xmms2})
endif(BUILD_XMMS2)

if(BUILD_PORT_MONITORS)
	set(port_monitors libtcp-portmon.c libtcp-portmon.h tcp-portmon.c tcp-portmon.h)
	set(optional_sources ${optional_sources} ${port_monitors})
endif(BUILD_PORT_MONITORS)

if(BUILD_X11)
	set(x11 x11.c x11.h fonts.c fonts.h)
	set(optional_sources ${optional_sources} ${x11})
endif(BUILD_X11)

if(BUILD_HDDTEMP)
	set(hddtemp hddtemp.c hddtemp.h)
	set(optional_sources ${optional_sources} ${hddtemp})
endif(BUILD_HDDTEMP)

if(BUILD_EVE)
	set(eve eve.c eve.h)
	set(optional_sources ${optional_sources} ${eve})
endif(BUILD_EVE)

if(BUILD_CURL)
	set(ccurl_thread ccurl_thread.c ccurl_thread.h)
	set(optional_sources ${optional_sources} ${ccurl_thread})
endif(BUILD_CURL)

if(BUILD_RSS)
	set(rss rss.c rss.h prss.c prss.h)
	set(optional_sources ${optional_sources} ${rss})
endif(BUILD_RSS)

if(BUILD_WEATHER)
	set(weather weather.c weather.h)
	set(optional_sources ${optional_sources} ${weather})
endif(BUILD_WEATHER)

if(BUILD_LUA)
	set(lua llua.c llua.h)
	set(optional_sources ${optional_sources} ${lua})
endif(BUILD_LUA)

if(BUILD_NVIDIA)
	set(nvidia nvidia.c nvidia.h)
	set(optional_sources ${optional_sources} ${nvidia})
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
	set(imlib2 imlib2.c imlib2.h)
	set(optional_sources ${optional_sources} ${imlib2})
endif(BUILD_IMLIB2)

if(BUILD_APCUPSD)
	set(apcupsd apcupsd.c apcupsd.h)
	set(optional_sources ${optional_sources} ${apcupsd})
endif(BUILD_APCUPSD)

if(BUILD_ICONV)
	set(iconv iconv_tools.c iconv_tools.h)
	set(optional_sources ${optional_sources} ${iconv})
endif(BUILD_ICONV)

add_executable(conky ${conky_sources} ${optional_sources})

target_link_libraries(conky ${conky_libs})
