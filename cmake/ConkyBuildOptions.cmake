# vim: ts=4 sw=4 noet ai cindent syntax=cmake

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
		FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# some extra debug flags
set(CMAKE_C_FLAGS_DEBUG "-ggdb -Wall -W -Wextra -Wunused -Wdeclaration-after-statement -Wundef -Wendif-labels -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Winline -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -std=c99 -pedantic -Werror" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Wall -W -Wextra -Wunused -std=c++0x -pedantic -Werror" CACHE STRING "Flags used by the compiler during debug builds." FORCE)


if(CMAKE_BUILD_TYPE MATCHES "Debug")
	set(DEBUG true)
endif(CMAKE_BUILD_TYPE MATCHES "Debug")

option(RELEASE "Build release package" false)
mark_as_advanced(RELEASE)

# Some standard options
set(SYSTEM_CONFIG_FILE "/etc/conky/conky.conf" CACHE STRING "Default system-wide Conky configuration file")
# use FORCE below to make sure this changes when CMAKE_INSTALL_PREFIX is modified
set(PACKAGE_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib/conky" CACHE STRING "Package library path (where Lua bindings are installed" FORCE)
set(DEFAULTNETDEV "eth0" CACHE STRING "Default networkdevice")
set(CONFIG_FILE "$HOME/.conkyrc" CACHE STRING "Configfile of the user")
set(MAX_SPECIALS_DEFAULT "512" CACHE STRING "Default maximum number of special things, e.g. fonts, offsets, aligns, etc.")
set(MAX_USER_TEXT_DEFAULT "16384" CACHE STRING "Default maximum size of config TEXT buffer, i.e. below TEXT line.")
set(DEFAULT_TEXT_BUFFER_SIZE "256" CACHE STRING "Default size used for temporary, static text buffers")
set(MAX_NET_INTERFACES "16" CACHE STRING "Maximum number of network devices")


# Platform specific options
# Linux only
if(OS_LINUX)
	option(BUILD_PORT_MONITORS "Build TCP portmon support" true)
	option(BUILD_IBM "Support for IBM/Lenovo notebooks" true)
	option(BUILD_HDDTEMP "Support for hddtemp" true)
	# nvidia may also work on FreeBSD, not sure
	option(BUILD_NVIDIA "Enable nvidia support" false)
else(OS_LINUX)
	set(BUILD_PORT_MONITORS false)
	set(BUILD_IBM false)
	set(BUILD_HDDTEMP false)
	set(BUILD_NVIDIA false)
endif(OS_LINUX)

# Optional features etc
option(BUILD_X11 "Build X11 support" true)
if(BUILD_X11)
	option(OWN_WINDOW "Enable own_window support" true)
	option(BUILD_XDAMAGE "Build Xdamage support" true)
	option(BUILD_XDBE "Build Xdbe (double-buffer) support" true)
	option(BUILD_XFT "Build Xft (freetype fonts) support" true)
endif(BUILD_X11)

option(BUILD_LUA "Build Lua support" true)

option(BUILD_AUDACIOUS "Build audacious (music player) support" false)
if(BUILD_AUDACIOUS)
	option(BUILD_AUDACIOUS_LEGACY "Use legacy audacious (music player) support" false)
else(BUILD_AUDACIOUS)
	set(BUILD_AUDACIOUS_LEGACY false)
endif(BUILD_AUDACIOUS)

option(BUILD_BMPX "Build BMPx (music player) support" false)

option(BUILD_MPD "Enable if you want MPD (music player) support" true)

option(BUILD_MOC "Enable if you want MOC (music player) support" true)

option(BUILD_XMMS2 "Enable if you want XMMS2 (music player) support" false)

option(BUILD_EVE "Enable if you want Eve-Online skill monitoring support" false)

option(BUILD_CURL "Enable if you want Curl support" false)

option(BUILD_RSS "Enable if you want RSS support" false)

option(BUILD_WEATHER_METAR "Enable METAR weather support" false)
option(BUILD_WEATHER_XOAP "Enable XOAP weather support" false)
if(BUILD_WEATHER_METAR OR BUILD_WEATHER_XOAP)
	set(BUILD_CURL true)
endif(BUILD_WEATHER_METAR OR BUILD_WEATHER_XOAP)

