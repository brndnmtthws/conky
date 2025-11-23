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

cmake_minimum_required(VERSION 3.17)

project(conky)

# This is the directory for our custom CMake modules.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# 'core' CMake stuff
include(Conky)

# Set project version, languages
project(conky VERSION ${RELEASE_VERSION} LANGUAGES C CXX)

# Handle build options
include(ConkyBuildOptions)

# Do platform checks
include(ConkyPlatformChecks)

# CPack module for installation tasks
include(ConkyCPackSetup)

# Use compilation cache (if enabled)
include(CCache)

# setup our configuration headers
configure_file(${CMAKE_MODULE_PATH}/config.h.in ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_MODULE_PATH}/build.h.in ${CMAKE_BINARY_DIR}/build.h)

set(conky_sources ${CMAKE_BINARY_DIR}/config.h ${CMAKE_BINARY_DIR}/build.h)

# Include 3rdparty code
add_subdirectory(3rdparty)
set(conky_libs ${conky_libs} toluapp_lib_static)
set(conky_libs ${conky_libs} Vc)

set(conky_includes ${conky_includes} "${CMAKE_SOURCE_DIR}/3rdparty/Vc")

# Finally, add some code
add_subdirectory(lua)
add_subdirectory(data)
add_subdirectory(doc)

if(BUILD_EXTRAS)
  add_subdirectory(extras)
endif(BUILD_EXTRAS)

if(BUILD_TESTING)
  if(CODE_COVERAGE)
    # Enable coverage checks
    include(CodeCoverage)
    append_coverage_compiler_flags()
  endif()

  include(Catch)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
  enable_testing()
endif()

if(NOT DEFINED DOC_PATH)
  set(DOC_PATH "share/doc/${CPACK_PACKAGE_NAME}-${RELEASE_VERSION}")
endif(NOT DEFINED DOC_PATH)

set(DOC_FILES extras/convert.lua data/conky_no_x11.conf data/conky.conf)

install(FILES ${DOC_FILES} DESTINATION ${DOC_PATH})

if(CHECK_CODE_QUALITY)
  find_package(ClangTidy)
  find_package(ClangFormat)
endif(CHECK_CODE_QUALITY)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_MODULE_PATH}/UninstallConky.cmake.in"
    "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake)
endif()
