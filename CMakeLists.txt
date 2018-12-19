# Copyright (C) 2007-2012 LuaDist.
# Created by Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

project ( toluapp C )
cmake_minimum_required ( VERSION 3.4 )
include ( cmake/dist.cmake )

include(FindPkgConfig)

pkg_search_module(LUA REQUIRED lua>=5.3 lua5.3 lua-5.3 lua53 lua5.2 lua-5.2 lua52 lua5.1 lua-5.1 lua51       lua>=5.1 luajit)
include_directories ( include src/lib ${LUA_INCLUDE_DIRS} )

# Build lib
file ( GLOB SRC_LIBTOLUAPP src/lib/*.c )
if ( MSVC )
  set ( DEF_FILE libtoluapp.def )
endif ( )

add_library ( toluapp_lib ${SRC_LIBTOLUAPP} ${DEF_FILE} )
target_link_libraries ( toluapp_lib ${LUA_LIBRARIES} )
set_target_properties ( toluapp_lib PROPERTIES OUTPUT_NAME toluapp CLEAN_DIRECT_OUTPUT
  1 )

# Build app
include_directories ( src/bin )
set ( SRC_TOLUA src/bin/tolua.c src/bin/toluabind.c )
add_executable ( toluapp ${SRC_TOLUA} )
target_link_libraries ( toluapp toluapp_lib ${LUA_LIBRARIES} )

# Install
install_library ( toluapp_lib )
install_executable ( toluapp )
install_header ( include/ )
install_data ( README INSTALL )
install_doc ( doc/ )
