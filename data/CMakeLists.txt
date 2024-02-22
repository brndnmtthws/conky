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

if(BUILD_BUILTIN_CONFIG)
  # generate defconfig
  if(BUILD_X11)
    set(builtin_config conky.conf)
  else(BUILD_X11)
    set(builtin_config conky_no_x11.conf)
  endif(BUILD_X11)

  add_custom_command(OUTPUT defconfig.h
    COMMAND ${CMAKE_SOURCE_DIR}/bin/text2c.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/${builtin_config}
    defconfig.h defconfig
    DEPENDS ${builtin_config})
  set(generated_hdr_files ${generated_hdr_files} defconfig.h)
endif(BUILD_BUILTIN_CONFIG)

if(BUILD_OLD_CONFIG)
  add_custom_command(OUTPUT convertconf.h
    COMMAND ${CMAKE_SOURCE_DIR}/bin/text2c.sh
    ${CMAKE_SOURCE_DIR}/extras/convert.lua
    convertconf.h convertconf
    DEPENDS ${CMAKE_SOURCE_DIR}/extras/convert.lua)
  set(generated_hdr_files ${generated_hdr_files} convertconf.h)
endif(BUILD_OLD_CONFIG)

add_custom_target(generated_hdr_files DEPENDS ${generated_hdr_files})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  install(FILES conky.desktop DESTINATION share/applications)
  install(FILES logo/conky-logomark-violet.svg
    DESTINATION share/icons/hicolor/scalable/apps)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
