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
add_subdirectory(lua)
add_subdirectory(data)
add_subdirectory(doc)
add_subdirectory(src)

# Install README into share dir (is this even the right spot? need to revisit
# this at some point)
install(FILES
	README
	DESTINATION share/doc/conky
)

