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

function(wrap_tolua VAR)
if(NOT ARGN)
	message(SEND_ERROR "Error: wrap_tolua called without any files")
	return()
endif(NOT ARGN)

SET(INCL)
SET(${VAR})
FOREACH(FIL ${ARGN})
	GET_FILENAME_COMPONENT(ABS_FIL ${FIL} ABSOLUTE)
	GET_FILENAME_COMPONENT(FIL_WE ${FIL} NAME_WE)
	LIST(APPEND ${VAR} "${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c")

	ADD_CUSTOM_COMMAND( OUTPUT ${${VAR}} ${INCL} COMMAND ${APP_TOLUA} -n
		${FIL_WE} -o ${CMAKE_CURRENT_BINARY_DIR}/lib${FIL_WE}.c ${ABS_FIL} DEPENDS
		${ABS_FIL} COMMENT "Running tolua++ on ${FIL}"
		VERBATIM )

	SET_SOURCE_FILES_PROPERTIES(${${VAR}} ${INCL} PROPERTIES GENERATED TRUE)
ENDFOREACH(FIL)

SET(${VAR} ${${VAR}} PARENT_SCOPE)

endfunction(wrap_tolua)
