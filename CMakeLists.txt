# vim: ts=4 sw=4 noet ai cindent syntax=cmake

cmake_minimum_required(VERSION 2.6)

project(conky)

# This is the directory for our custom CMake modules.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# 'core' CMake stuff
include(Conky)

# Handle build options
include(ConkyBuildOptions)

# Do platform checks
include(ConkyPlatformChecks)

# CPack module for installation tasks
include(ConkyCPackSetup)

# setup our configuration headers
configure_file(${CMAKE_MODULE_PATH}/config.h.in ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_MODULE_PATH}/build.h.in ${CMAKE_BINARY_DIR}/build.h)

# Finally, add some code
add_subdirectory(src)
add_subdirectory(lua)
add_subdirectory(doc)

# Install README into share dir (is this even the right spot? need to revisit
# this at some point)
install(FILES
	README
	DESTINATION share/doc/conky
)

