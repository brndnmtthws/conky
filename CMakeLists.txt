# vim: ts=4 sw=4 noet ai cindent syntax=cmake
#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2010 Brenden Matthews, et. al. (see AUTHORS)
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.5)

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

if(CHECK_CODE_FORMAT)
  # Set up clang-format
  set(CLANG_FORMAT_BIN_NAME clang-format clang-format-5.0)
  set(CLANG_FORMAT_EXCLUDE_PATTERNS  "build/" ${CMAKE_BINARY_DIR})
  find_package(ClangFormat)
endif(CHECK_CODE_FORMAT)

# setup our configuration headers
configure_file(${CMAKE_MODULE_PATH}/config.h.in ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_MODULE_PATH}/build.h.in ${CMAKE_BINARY_DIR}/build.h)

set(conky_sources
  ${CMAKE_BINARY_DIR}/config.h
  ${CMAKE_BINARY_DIR}/build.h
)

# Finally, add some code
add_subdirectory(lua)
add_subdirectory(data)
add_subdirectory(doc)
add_subdirectory(src)

IF(NOT DEFINED DOC_PATH)
  set(DOC_PATH "share/doc/${CPACK_PACKAGE_NAME}-${VERSION}")
ENDIF(NOT DEFINED DOC_PATH)
set(DOC_FILES extras/convert.lua data/conky_no_x11.conf data/conky.conf)

set(HTML_PATH "${DOC_PATH}/html")
set(HTML_FILES doc/config_settings.html doc/docs.html doc/lua.html doc/variables.html)

set(MAN_PATH "share/man/man1")
set(MAN_FILES doc/conky.1)

install(FILES ${DOC_FILES}
  DESTINATION ${DOC_PATH})

if(MAINTAINER_MODE)
  install(FILES ${HTML_FILES}
    DESTINATION ${HTML_PATH})

  install(FILES ${MAN_FILES}
    DESTINATION ${MAN_PATH})
endif(MAINTAINER_MODE)
