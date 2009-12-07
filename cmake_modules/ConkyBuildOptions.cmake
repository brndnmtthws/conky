# vim: ts=4 sw=4 noet ai cindent syntax=cmake

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
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	option(BUILD_PORT_MONITORS "Build TCP portmon support" true)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

# Optional features etc
option(BUILD_X11 "Build X11 support" true)
if(BUILD_X11)
	option(OWN_WINDOW "Enable own_window support" true)
	option(BUILD_XDAMAGE "Build Xdamage support" true)
	option(BUILD_XDBE "Build Xdbe (double-buffer) support" true)
	option(BUILD_XFT "Build Xft (freetype fonts) support" true)
endif(BUILD_X11)

option(BUILD_LUA "Build Lua support" true)

option(BUILD_AUDACIOUS "Build audacious player support" false)
if(BUILD_AUDACIOUS)
	option(BUILD_AUDACIOUS_LEGACY "Use legacy audacious player support" false)
else(BUILD_AUDACIOUS)
	set(BUILD_AUDACIOUS_LEGACY false)
endif(BUILD_AUDACIOUS)

