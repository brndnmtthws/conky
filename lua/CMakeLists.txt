# vim: ts=4 sw=4 noet ai cindent syntax=cmake
#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2012 Brenden Matthews, et. al. (see AUTHORS)
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

include(ToLua)

add_definitions(-DTOLUA_RELEASE)
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_C_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")

if(BUILD_LUA_CAIRO)
	include_directories(${luacairo_includes} ${CMAKE_CURRENT_SOURCE_DIR})
	# cairo_set_dash() needs this special hack to work properly
	# if you have a better solution, please let me know
	wrap_tolua(luacairo_src cairo.pkg libcairo.patch)

	add_library(conky-cairo SHARED ${luacairo_src})
	set_target_properties(conky-cairo PROPERTIES OUTPUT_NAME "cairo")

	target_link_libraries(conky-cairo ${luacairo_libs} ${TOLUA_LIBS})
	set(lua_libs ${lua_libs} conky-cairo)
endif(BUILD_LUA_CAIRO)

if(BUILD_LUA_IMLIB2)
	include_directories(${luaimlib2_includes} ${CMAKE_CURRENT_SOURCE_DIR})
	wrap_tolua(luaimlib2_src imlib2.pkg)

	add_library(conky-imlib2 SHARED ${luaimlib2_src})
	set_target_properties(conky-imlib2 PROPERTIES OUTPUT_NAME "imlib2")

	target_link_libraries(conky-imlib2 ${luaimlib2_libs} ${TOLUA_LIBS})
	set(lua_libs ${lua_libs} conky-imlib2)
endif(BUILD_LUA_IMLIB2)

if(BUILD_LUA_RSVG)
	include_directories(${luarsvg_includes} ${CMAKE_CURRENT_SOURCE_DIR})
	wrap_tolua(luarsvg_src rsvg.pkg)

	add_library(conky-rsvg SHARED ${luarsvg_src})
	set_target_properties(conky-rsvg PROPERTIES OUTPUT_NAME "rsvg")

	target_link_libraries(conky-rsvg ${luarsvg_libs} ${TOLUA_LIBS})
	set(lua_libs ${lua_libs} conky-rsvg)
endif(BUILD_LUA_RSVG)

if(BUILD_LUA_CAIRO AND BUILD_LUA_IMLIB2)
	include_directories(${luacairo_includes} ${luaimlib2_includes} ${CMAKE_CURRENT_SOURCE_DIR})
	wrap_tolua(luacairo_imlib2_helper_src cairo_imlib2_helper.pkg)

	add_library(conky-cairo_imlib2_helper SHARED ${luacairo_imlib2_helper_src})
	set_target_properties(conky-cairo_imlib2_helper PROPERTIES OUTPUT_NAME "cairo_imlib2_helper")

	target_link_libraries(conky-cairo_imlib2_helper ${luacairo_libs} ${luaimlib2_libs} ${TOLUA_LIBS})
	set(lua_libs ${lua_libs} conky-cairo_imlib2_helper)
endif(BUILD_LUA_CAIRO AND BUILD_LUA_IMLIB2)

install(TARGETS
	${lua_libs}
	LIBRARY DESTINATION lib/conky
	ARCHIVE DESTINATION lib/conky
)
