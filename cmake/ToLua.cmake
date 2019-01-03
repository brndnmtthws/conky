#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2018 Brenden Matthews, et. al. (see AUTHORS) All rights
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

function(wrap_tolua VAR FIL)
  set(INCL)
  set(${VAR})

  get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
  get_filename_component(FIL_WE ${FIL} NAME_WE)
  list(APPEND ${VAR} "${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c")

  if(DEFINED ARGV2)
    get_filename_component(PATCH ${ARGV2} ABSOLUTE)
    set(TOLUA_OUT ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}-orig.c)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c
                       COMMAND ${APP_PATCH} -s ${TOLUA_OUT} ${PATCH} -o
                               ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c
                       DEPENDS ${TOLUA_OUT} ${PATCH}
                       COMMENT "Patching lib${FIL_WE}-orig.c"
                       VERBATIM)
    set_source_files_properties(${TOLUA_OUT} PROPERTIES GENERATED TRUE)
  else()
    set(TOLUA_OUT ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c)
  endif(DEFINED ARGV2)

  # Call toluapp from 3rdparty/ path directly. The last argument to toluapp is
  # the path to the tolua Lua sources.
  add_custom_command(OUTPUT ${TOLUA_OUT} ${INCL}
                     COMMAND toluapp -n ${FIL_WE} -o ${TOLUA_OUT} ${ABS_FIL}
                             ${CMAKE_SOURCE_DIR}/3rdparty/toluapp/src/bin/lua/
                     DEPENDS ${ABS_FIL}
                     COMMENT "Running tolua++ on ${FIL}"
                     VERBATIM)

  set_source_files_properties(${${VAR}} ${INCL} PROPERTIES GENERATED TRUE)
  set_source_files_properties(
    ${${VAR}}
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-bad-function-cast -Wno-unused-parameter -Wno-cast-qual -Wno-error=pedantic"
    )

  set(${VAR} ${${VAR}} PARENT_SCOPE)

endfunction(wrap_tolua)
