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

function(wrap_tolua VAR FIL)
SET(INCL)
SET(${VAR})

GET_FILENAME_COMPONENT(ABS_FIL ${FIL} ABSOLUTE)
GET_FILENAME_COMPONENT(FIL_WE ${FIL} NAME_WE)
LIST(APPEND ${VAR} "${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c")

if(DEFINED ARGV2)
	GET_FILENAME_COMPONENT(PATCH ${ARGV2} ABSOLUTE)
	SET(TOLUA_OUT ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}-orig.c)
	ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c
		COMMAND patch -s ${TOLUA_OUT} ${PATCH} -o ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c
		DEPENDS ${TOLUA_OUT} ${PATCH}
		COMMENT "Patching lib${FIL_WE}-orig.c"
		VERBATIM)
	SET_SOURCE_FILES_PROPERTIES(${TOLUA_OUT} PROPERTIES GENERATED TRUE)
else()
	SET(TOLUA_OUT ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c)
endif(DEFINED ARGV2)

ADD_CUSTOM_COMMAND( OUTPUT ${TOLUA_OUT} ${INCL} COMMAND ${APP_TOLUA} -n
	${FIL_WE} -o ${TOLUA_OUT} ${ABS_FIL} DEPENDS
	${ABS_FIL} COMMENT "Running tolua++ on ${FIL}"
	VERBATIM )

SET_SOURCE_FILES_PROPERTIES(${${VAR}} ${INCL} PROPERTIES GENERATED TRUE)


SET(${VAR} ${${VAR}} PARENT_SCOPE)

endfunction(wrap_tolua)
