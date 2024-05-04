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

include(ToLua)

add_definitions(-DTOLUA_RELEASE)

# NOTE: Don't chain options in IFs here, their dependency is already handled by
# ConkyBuildOptions.cmake
if(BUILD_LUA_CAIRO)
  include_directories(${luacairo_includes} ${CMAKE_CURRENT_SOURCE_DIR})

  # cairo_set_dash() needs this special hack to work properly if you have a
  # better solution, please let me know
  wrap_tolua(luacairo_src cairo.pkg libcairo.patch)

  add_library(conky-cairo MODULE ${luacairo_src})
  set_target_properties(conky-cairo PROPERTIES OUTPUT_NAME "cairo")

  target_link_libraries(conky-cairo ${luacairo_libs} toluapp_lib_static)
  set(lua_libs ${lua_libs} conky-cairo)

  print_target_properties(conky-cairo)
endif(BUILD_LUA_CAIRO)

if(BUILD_LUA_CAIRO_XLIB)
  include_directories(${luacairoxlib_includes} ${CMAKE_CURRENT_SOURCE_DIR})

  # Need the directory for -lSM
  get_filename_component(X11_SM_LIB_PATH ${X11_SM_LIB} DIRECTORY)
  link_directories(${X11_SM_LIB_PATH})
  wrap_tolua(luacairoxlib_src cairo_xlib.pkg)

  add_library(conky-cairo_xlib SHARED ${luacairoxlib_src})
  set_target_properties(conky-cairo_xlib PROPERTIES OUTPUT_NAME "cairo_xlib")

  target_link_libraries(conky-cairo_xlib
    ${luacairoxlib_libs}
    ${luacairo_libs}
    toluapp_lib_static)
  set(lua_libs ${lua_libs} conky-cairo_xlib)

  print_target_properties(conky-cairo_xlib)
endif(BUILD_LUA_CAIRO_XLIB)

if(BUILD_LUA_IMLIB2)
  include_directories(${luaimlib2_includes} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)

  if(IMLIB2_VERSION VERSION_GREATER_EQUAL "1.10.0")
    wrap_tolua(luaimlib2_src imlib2.pkg)
  else(IMLIB2_VERSION VERSION_GREATER_EQUAL "1.10.0")
    wrap_tolua(luaimlib2_src imlib2_old.pkg)
  endif(IMLIB2_VERSION VERSION_GREATER_EQUAL "1.10.0")

  add_library(conky-imlib2 MODULE ${luaimlib2_src})
  set_target_properties(conky-imlib2 PROPERTIES OUTPUT_NAME "imlib2")

  target_link_libraries(conky-imlib2 ${luaimlib2_libs} toluapp_lib_static )
  set(lua_libs ${lua_libs} conky-imlib2)

  print_target_properties(conky-imlib2)
endif(BUILD_LUA_IMLIB2)

if(BUILD_LUA_CAIRO AND BUILD_LUA_IMLIB2)
  include_directories(${luacairo_includes} ${luaimlib2_includes}
    ${CMAKE_CURRENT_SOURCE_DIR})
  wrap_tolua(luacairo_imlib2_helper_src cairo_imlib2_helper.pkg)

  set(imlib_helper_libs ${luacairo_libs} ${luaimlib2_libs} toluapp_lib_static)
  # if(BUILD_I18N)
  #   set(imlib_helper_libs ${imlib_helper_libs} -lintl)
  # endif()

  add_library(conky-cairo_imlib2_helper MODULE ${luacairo_imlib2_helper_src})
  set_target_properties(conky-cairo_imlib2_helper
    PROPERTIES OUTPUT_NAME "cairo_imlib2_helper")

  target_link_libraries(conky-cairo_imlib2_helper ${imlib_helper_libs})
  set(lua_libs ${lua_libs} conky-cairo_imlib2_helper)
endif(BUILD_LUA_CAIRO AND BUILD_LUA_IMLIB2)

if(BUILD_LUA_RSVG)
  include_directories(${luarsvg_includes} ${CMAKE_CURRENT_SOURCE_DIR})
  wrap_tolua(luarsvg_src rsvg.pkg)

  add_library(conky-rsvg MODULE ${luarsvg_src})
  set_target_properties(conky-rsvg PROPERTIES OUTPUT_NAME "rsvg")

  target_link_libraries(conky-rsvg ${luarsvg_libs} toluapp_lib_static)
  set(lua_libs ${lua_libs} conky-rsvg)

  print_target_properties(conky-rsvg)
endif(BUILD_LUA_RSVG)

install(TARGETS ${lua_libs}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}/conky
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}/conky)
