#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2019 Brenden Matthews, et. al. (see AUTHORS) All rights
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

cmake_minimum_required(VERSION 3.6)

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

set(conky_sources ${CMAKE_BINARY_DIR}/config.h ${CMAKE_BINARY_DIR}/build.h)

# Finally, add some code
add_subdirectory(lua)
add_subdirectory(data)
add_subdirectory(doc)

# Include 3rdparty toluapp
add_subdirectory(3rdparty/toluapp)
set(conky_libs ${conky_libs} toluapp_lib_static)

if(BUILD_TESTS)
  if(USING_CLANG_7)
    message(STATUS "Detected clang-7, enabling test coverage reports")
    # Enable coverage checks
    include(CodeCoverage)
    append_coverage_compiler_flags()
  else()
    message(STATUS "NOT enabling test coverage reports")
  endif()
  include(Catch)
endif()

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(tests)
  enable_testing()
endif()

if(NOT DEFINED DOC_PATH)
  set(DOC_PATH "share/doc/${CPACK_PACKAGE_NAME}-${VERSION}")
endif(NOT DEFINED DOC_PATH)
set(DOC_FILES extras/convert.lua data/conky_no_x11.conf data/conky.conf)

set(HTML_PATH "${DOC_PATH}/html")
set(HTML_FILES
    doc/config_settings.html
    doc/docs.html
    doc/lua.html
    doc/variables.html)

set(MAN_PATH "share/man/man1")
set(MAN_FILES doc/conky.1)

install(FILES ${DOC_FILES} DESTINATION ${DOC_PATH})

if(MAINTAINER_MODE)
  install(FILES ${HTML_FILES} DESTINATION ${HTML_PATH})

  install(FILES ${MAN_FILES} DESTINATION ${MAN_PATH})
endif(MAINTAINER_MODE)

if(CHECK_CODE_QUALITY)
  # Set up clang-tidy
  set(CLANG_TIDY_BIN_NAME
      clang-tidy
      clang-tidy-5.0
      clang-tidy-6.0
      clang-tidy-7.0)
  set(CLANG_TIDY_EXCLUDE_PATTERNS "build/" ${CMAKE_BINARY_DIR})
  find_package(ClangTidy)
endif(CHECK_CODE_QUALITY)
