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

add_library(lua-common INTERFACE)
target_include_directories(lua-common INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_SOURCE_DIR}/3rdparty/toluapp/include"
)
target_compile_definitions(lua-common INTERFACE -DTOLUA_RELEASE)

macro(ADD_LUA_MODULE TARGET)
  add_library(${TARGET} MODULE ${ARGN}) # add_library(${TARGET} MODULE sources)
  string(REPLACE "conky-" "" TARGET_MODULE_NAME "${TARGET}")
  set_target_properties(${TARGET} PROPERTIES
    OUTPUT_NAME "${TARGET_MODULE_NAME}" # remove conky- prefix from require name
    LINKER_LANGUAGE CXX                 # allow stub modules
  )
  unset(TARGET_MODULE_NAME)
  unset(__conky_prefix)
  target_link_libraries(${TARGET}
    PUBLIC lua-common # include common configuration
    PRIVATE conky-core toluapp_lib_static
  )
  list(APPEND lua-modules ${TARGET})
endmacro()

# NOTE: Don't chain options in IFs here, their dependency is already handled by
# ConkyBuildOptions.cmake

if(BUILD_LUA_CAIRO)
  # cairo_set_dash() needs this special hack to work properly if you have a
  # better solution, please let me know
  wrap_tolua(cairo.pkg
    PATCH libcairo.patch
    luacairo_src
  )
  add_lua_module(conky-cairo ${luacairo_src})
endif()

# Pack additional Xlib specific functions into cairo module if Xlib is enabled.
if(BUILD_LUA_CAIRO_XLIB)
  # Need the directory for -lSM
  cmake_path(GET X11_SM_LIB PARENT_PATH X11_SM_LIB_DIR)
  target_link_directories(conky-cairo PRIVATE ${X11_SM_LIB_DIR})

  wrap_tolua(cairo_xlib.pkg luacairoxlib_src)
  target_sources(conky-cairo PUBLIC ${luacairoxlib_src})
  target_link_libraries(conky-cairo PRIVATE ${luacairoxlib_libs})
  print_target_properties(conky-cairo)
else()
  print_target_properties(conky-cairo)
endif()

# Kept for compatibility with intermediate releases that separated some
# X11 specific functions into a separate Lua require.
# Will be removed in 2028.
if(BUILD_LUA_CAIRO_XLIB)
  add_lua_module(conky-cairo_xlib "${CMAKE_CURRENT_SOURCE_DIR}/empty")
  set_target_properties(conky-cairo_xlib PROPERTIES OUTPUT_NAME "cairo_xlib")
endif()

if(BUILD_LUA_IMLIB2)
  if(IMLIB2_VERSION VERSION_GREATER_EQUAL "1.10.0") # >= 1.10.0
    wrap_tolua(imlib2.pkg luaimlib2_src)
  else() # < 1.10.0
    wrap_tolua(imlib2_old.pkg luaimlib2_src)
  endif()

  add_lua_module(conky-imlib2 ${luaimlib2_src})
  target_link_libraries(conky-imlib2 PRIVATE ${luaimlib2_libs})
  print_target_properties(conky-imlib2)
endif()

if(BUILD_LUA_CAIRO AND BUILD_LUA_IMLIB2)
  wrap_tolua(cairo_imlib2_helper.pkg luacairo_imlib2_helper_src)

  add_lua_module(conky-cairo_imlib2_helper ${luacairo_imlib2_helper_src})
  target_link_libraries(conky-cairo_imlib2_helper
    PRIVATE ${luacairo_libs} ${luaimlib2_libs}
  )
  print_target_properties(conky-cairo_imlib2_helper)
endif()

if(BUILD_LUA_RSVG)
  include_directories(${luarsvg_includes} ${CMAKE_CURRENT_SOURCE_DIR})
  wrap_tolua(rsvg.pkg luarsvg_src)

  add_lua_module(conky-rsvg ${luarsvg_src})
  target_link_libraries(conky-rsvg PRIVATE ${luarsvg_libs})
  print_target_properties(conky-rsvg)
endif()

install(TARGETS ${lua-modules}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}/conky
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}/conky
)
