# Created under MIT License 2007-2012 for LuaDist by Peter Kapec
# Copy of the license is available in the COPYRIGHT file.
# Vendored and maintained for conky build process.

project(toluapp C)
cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED LUA_VERSION)
  set(LUA_VERSION "5.3")
endif()

include(FindLua ${LUA_VERSION})
find_package(Lua ${LUA_VERSION} REQUIRED)

# Build lib
file(GLOB SRC_LIBTOLUAPP "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/*.c")
if(MSVC)
  list(APPEND SRC_LIBTOLUAPP libtoluapp.def)
endif()

add_library(toluapp_lib_static STATIC ${SRC_LIBTOLUAPP})
target_include_directories(toluapp_lib_static
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE ${LUA_INCLUDE_DIR}
)
target_link_libraries(toluapp_lib_static PUBLIC ${LUA_LIBRARIES})
set_target_properties(toluapp_lib_static PROPERTIES
  COMPILE_FLAGS -fPIC # required for static linking
  OUTPUT_NAME toluapp
  CLEAN_DIRECT_OUTPUT 1
)

# Build app
set(SRC_TOLUA
  src/bin/tolua.c
  src/bin/toluabind.c
)
add_executable(toluapp ${SRC_TOLUA})
target_link_libraries(toluapp toluapp_lib_static)
