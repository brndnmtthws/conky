# Copyright (C) 2007-2009 LuaDist.
# Created by Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

PROJECT (toluapp C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
INCLUDE(dist.cmake)

# Find Lua
find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# Build lib
INCLUDE_DIRECTORIES(include src/lib)
FILE(GLOB SRC_LIBTOLUAPP src/lib/*.c )
IF(MSVC)
	SET(DEF_FILE libtoluapp.def)
ENDIF()

ADD_LIBRARY (toluapp_lib SHARED ${SRC_LIBTOLUAPP} ${DEF_FILE})
TARGET_LINK_LIBRARIES (toluapp_lib ${LUA_LIBRARIES})
SET_TARGET_PROPERTIES (toluapp_lib PROPERTIES OUTPUT_NAME toluapp CLEAN_DIRECT_OUTPUT 1 )

# Build app
INCLUDE_DIRECTORIES(src/bin)
SET(SRC_TOLUA src/bin/tolua.c src/bin/toluabind.c)
ADD_EXECUTABLE (toluapp ${SRC_TOLUA})
TARGET_LINK_LIBRARIES (toluapp toluapp_lib ${LUA_LIBRARIES})

# Install
INSTALL(TARGETS toluapp toluapp_lib RUNTIME DESTINATION ${INSTALL_BIN} LIBRARY DESTINATION ${INSTALL_LIB} ARCHIVE DESTINATION ${INSTALL_LIB})
INSTALL(DIRECTORY include/  DESTINATION ${INSTALL_INC})
INSTALL(FILES README INSTALL DESTINATION ${INSTALL_DATA})
INSTALL(DIRECTORY doc/ DESTINATION ${INSTALL_DOC})
