# vim: ts=4 sw=4 noet ai cindent syntax=cmake

cmake_minimum_required(VERSION 2.6)

project(conky)

# This is the directory for our custom CMake modules.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules)

# 'core' CMake stuff
include(Conky)

# Handle build options
include(ConkyBuildOptions)

# Do platform checks
include(ConkyPlatformChecks)

# CPack module for installation tasks
include(ConkyCPackSetup)

# Finally, add some code
add_subdirectory(src)
add_subdirectory(lua)
add_subdirectory(doc)

