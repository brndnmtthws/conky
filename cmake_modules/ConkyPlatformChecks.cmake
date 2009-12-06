# vim: ts=4 sw=4 noet ai cindent syntax=cmake

include(FindPkgConfig)

check_include_files(sys/statfs.h HAVE_SYS_STATFS_H)
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/inotify.h HAVE_SYS_INOTIFY_H)

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
		find_path(freetype_INCLUDE_PATH freetype/config/ftconfig.h /usr/include
			/usr/local/include /usr/include/freetype2
			/usr/local/include/freetype2)
		if(freetype_INCLUDE_PATH)
			set(freetype_FOUND TRUE)
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
			set(X11_Xdbe_FOUND TRUE)
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

if(BUILD_PORT_MONITORS)
	set(WANT_GLIB true)
endif(BUILD_PORT_MONITORS)

if(WANT_GLIB)
	pkg_check_modules(GLIB REQUIRED glib-2.0)
	set(conky_libs ${conky_libs} ${GLIB_LIBRARIES})
	set(conky_includes ${conky_includes} ${GLIB_INCLUDE_DIRS})
endif(WANT_GLIB)

