# vim: ts=4 sw=4 noet ai cindent syntax=cmake

include(FindPkgConfig)
include(CheckFunctionExists)
include(CheckIncludeFile)

# Check for some headers
check_include_files(sys/statfs.h HAVE_SYS_STATFS_H)
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/inotify.h HAVE_SYS_INOTIFY_H)
check_include_files(dirent.h HAVE_DIRENT_H)

# Check for some functions
check_function_exists(strndup HAVE_STRNDUP)

# standard path to search for includes
set(INCLUDE_SEARCH_PATH /usr/include /usr/local/include)

# Set system vars
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(OS_LINUX true)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(OS_FREEBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")

if(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
	set(OS_OPENBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")

if(CMAKE_SYSTEM_NAME MATCHES "Solaris")
	set(OS_SOLARIS true)
endif(CMAKE_SYSTEM_NAME MATCHES "Solaris")

if(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
	set(OS_NETBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")

if(NOT OS_LINUX AND NOT OS_FREEBSD AND NOT OS_OPENBSD)
	message(FATAL_ERROR "Your platform, '${CMAKE_SYSTEM_NAME}', is not currently supported.  Patches are welcome.")
endif(NOT OS_LINUX AND NOT OS_FREEBSD AND NOT OS_OPENBSD)

if(BUILD_MATH)
	set(conky_libs ${conky_libs} -lm)
endif(BUILD_MATH)

if(BUILD_CONFIG_OUTPUT)
	check_function_exists(fopencookie HAVE_FOPENCOOKIE)
	check_function_exists(funopen HAVE_FUNOPEN)
endif(BUILD_CONFIG_OUTPUT)

if(BUILD_NCURSES)
	check_include_file(ncurses.h NCURSES_H)
	find_library(NCURSES_LIB NAMES ncurses)
	if(NOT NCURSES_H OR NOT NCURSES_LIB)
		message(FATAL_ERROR "Unable to find ncurses library")
	endif(NOT NCURSES_H OR NOT NCURSES_LIB)
	set(conky_libs ${conky_libs} ${NCURSES_LIB})
endif(BUILD_NCURSES)

if(BUILD_WLAN)
	check_include_file(iwlib.h IWLIB_H)
	find_library(IWLIB_LIB NAMES iw)
	if(NOT IWLIB_H OR NOT IWLIB_LIB)
		message(FATAL_ERROR "Unable to find iwlib")
	endif(NOT IWLIB_H OR NOT IWLIB_LIB)
	set(CMAKE_REQUIRED_LIBRARIES ${IWLIB_LIB})
	check_function_exists(iw_sockets_open IWLIB_SOCKETS_OPEN_FUNC)
endif(BUILD_WLAN)

if(BUILD_PORT_MONITORS)
	check_function_exists(getnameinfo HAVE_GETNAMEINFO)
	if(NOT HAVE_GETNAMEINFO)
		message(FATAL_ERROR "could not find getnameinfo()")
	endif(NOT HAVE_GETNAMEINFO)
	check_include_files("netdb.h;netinet/in.h;netinet/tcp.h;sys/socket.h;arpa/inet.h" HAVE_PORTMON_HEADERS)
	if(NOT HAVE_PORTMON_HEADERS)
		message(FATAL_ERROR "missing needed network header(s) for port monitoring")
	endif(NOT HAVE_PORTMON_HEADERS)
endif(BUILD_PORT_MONITORS)


# Check for iconv
check_include_file(iconv.h HAVE_ICONV_H)
find_library(ICONV_LIBRARY NAMES iconv)
if(HAVE_ICONV_H AND ICONV_LIBRARY)
	set(conky_includes ${conky_includes} ${ICONV_INCLUDE_DIR})
	set(conky_libs ${conky_libs} ${ICONV_LIBRARY})
	set(HAVE_ICONV true)
else(HAVE_ICONV_H AND ICONV_LIBRARY)
	# too annoying
	# message(WARNING "Unable to find iconv library")
	set(HAVE_ICONV false)
endif(HAVE_ICONV_H AND ICONV_LIBRARY)



# check for Xlib
if(BUILD_X11)
	include(FindX11)
	find_package(X11)
	set(conky_includes ${conky_includes} ${X11_INCLUDE_DIR})
	set(conky_libs ${conky_libs} ${X11_LIBRARIES})

	# check for Xdamage
	if(BUILD_XDAMAGE)
		if(NOT X11_Xdamage_FOUND)
			message(FATAL_ERROR "Unable to find Xdamage library")
		endif(NOT X11_Xdamage_FOUND)
		if(NOT X11_Xfixes_FOUND)
			message(FATAL_ERROR "Unable to find Xfixes library")
		endif(NOT X11_Xfixes_FOUND)
		set(conky_libs ${conky_libs} ${X11_Xdamage_LIB} ${X11_Xfixes_LIB})
	endif(BUILD_XDAMAGE)

	# check for Xft
	if(BUILD_XFT)
		find_path(freetype_INCLUDE_PATH freetype/config/ftconfig.h ${INCLUDE_SEARCH_PATH}
			/usr/include/freetype2
			/usr/local/include/freetype2)
		if(freetype_INCLUDE_PATH)
			set(freetype_FOUND true)
			set(conky_includes ${conky_includes} ${freetype_INCLUDE_PATH})
		else(freetype_INCLUDE_PATH)
			message(FATAL_ERROR "Unable to find freetype library")
		endif(freetype_INCLUDE_PATH)
		if(NOT X11_Xft_FOUND)
			message(FATAL_ERROR "Unable to find Xft library")
		endif(NOT X11_Xft_FOUND)
		set(conky_libs ${conky_libs} ${X11_Xft_LIB})
	endif(BUILD_XFT)

	# check for Xdbe
	if(BUILD_XDBE)
		find_path(X11_Xdbe_INCLUDE_PATH X11/extensions/Xdbe.h ${X11_INC_SEARCH_PATH})
		if(X11_Xdbe_INCLUDE_PATH)
			set(X11_Xdbe_FOUND true)
			set(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xdbe_INCLUDE_PATH})
		endif(X11_Xdbe_INCLUDE_PATH)
		if(NOT X11_Xdbe_FOUND)
			message(FATAL_ERROR "Unable to find Xdbe library")
		endif(NOT X11_Xdbe_FOUND)
	endif(BUILD_XDBE)

endif(BUILD_X11)

if(BUILD_LUA)
	pkg_search_module(LUA REQUIRED lua>=5.1 lua-5.1>=5.1 lua5.1>=5.1)
	set(conky_libs ${conky_libs} ${LUA_LIBRARIES})
	set(conky_includes ${conky_includes} ${LUA_INCLUDE_DIRS})
endif(BUILD_LUA)

if(BUILD_AUDACIOUS)
	set(WANT_GLIB true)
	if(NOT BUILD_AUDACIOUS_LEGACY)
		pkg_check_modules(AUDACIOUS REQUIRED audacious>=1.4.0 dbus-glib-1 gobject-2.0)
		# do we need this below?
		#CPPFLAGS="$Audacious_CFLAGS -I`pkg-config --variable=audacious_include_dir audacious`/audacious"
		#AC_CHECK_HEADERS([audacious/audctrl.h audacious/dbus.h glib.h glib-object.h],
		#                 [], AC_MSG_ERROR([required header(s) not found]))
	else(NOT BUILD_AUDACIOUS_LEGACY)
		pkg_check_modules(AUDACIOUS REQUIRED audacious<1.4.0)
		# do we need this below?
		#CPPFLAGS="$Audacious_CFLAGS -I`pkg-config --variable=audacious_include_dir audacious`/audacious"
		#AC_CHECK_HEADERS([audacious/beepctrl.h glib.h], [], AC_MSG_ERROR([required  header(s) not found]))
		#CPPFLAGS="$save_CPPFLAGS"
	endif(NOT BUILD_AUDACIOUS_LEGACY)
endif(BUILD_AUDACIOUS)

if(BUILD_BMPX)
	pkg_check_modules(BMPX REQUIRED bmp-2.0>=0.14.0)
	set(conky_libs ${conky_libs} ${BMPX_LIBRARIES})
	set(conky_includes ${conky_includes} ${BMPX_INCLUDE_DIRS})
endif(BUILD_BMPX)

if(BUILD_XMMS2)
	pkg_check_modules(XMMS2 REQUIRED xmms2-client)
	set(conky_libs ${conky_libs} ${XMMS2_LIBRARIES})
	set(conky_includes ${conky_includes} ${XMMS2_INCLUDE_DIRS})
endif(BUILD_XMMS2)

if(BUILD_EVE)
	set(WANT_CURL true)
	set(WANT_LIBXML2 true)
endif(BUILD_EVE)

if(BUILD_CURL)
	set(WANT_CURL true)
endif(BUILD_CURL)

if(BUILD_RSS)
	set(WANT_CURL true)
	set(WANT_GLIB true)
	set(WANT_LIBXML2 true)
endif(BUILD_RSS)

if(BUILD_WEATHER_METAR)
	set(WANT_CURL true)
	set(BUILD_WEATHER true)
endif(BUILD_WEATHER_METAR)

if(BUILD_WEATHER_XOAP)
	set(WANT_LIBXML2 true)
	set(WANT_CURL true)
	set(BUILD_XOAP true)
	set(BUILD_WEATHER true)
endif(BUILD_WEATHER_XOAP)

if(BUILD_NVIDIA)
	find_path(XNVCtrl_INCLUDE_PATH NVCtrl/NVCtrl.h ${INCLUDE_SEARCH_PATH})
	find_library(XNVCtrl_LIB NAMES XNVCtrl)
	if(XNVCtrl_INCLUDE_PATH AND XNVCtrl_LIB)
		set(XNVCtrl_FOUND true)
		set(conky_libs ${conky_libs} ${XNVCtrl_LIB})
		set(conky_includes ${conky_includes} ${XNVCtrl_INCLUDE_PATH})
	else(XNVCtrl_INCLUDE_PATH AND XNVCtrl_LIB)
		message(FATAL_ERROR "Unable to find XNVCtrl library")
	endif(XNVCtrl_INCLUDE_PATH AND XNVCtrl_LIB)
endif(BUILD_NVIDIA)

if(BUILD_IMLIB2)
	pkg_check_modules(IMLIB2 imlib2)
	set(conky_libs ${conky_libs} ${IMLIB2_LIB})
	set(conky_includes ${conky_includes} ${IMLIB2_INCLUDE_PATH})
endif(BUILD_IMLIB2)

# Common libraries
if(WANT_GLIB)
	pkg_check_modules(GLIB REQUIRED glib-2.0)
	set(conky_libs ${conky_libs} ${GLIB_LIBRARIES})
	set(conky_includes ${conky_includes} ${GLIB_INCLUDE_DIRS})
endif(WANT_GLIB)

if(WANT_CURL)
	pkg_check_modules(CURL REQUIRED libcurl)
	set(conky_libs ${conky_libs} ${CURL_LIBRARIES})
	set(conky_includes ${conky_includes} ${CURL_INCLUDE_DIRS})
endif(WANT_CURL)

if(WANT_LIBXML2)
	pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
	set(conky_libs ${conky_libs} ${LIBXML2_LIBRARIES})
	set(conky_includes ${conky_includes} ${LIBXML2_INCLUDE_DIRS})
endif(WANT_LIBXML2)

