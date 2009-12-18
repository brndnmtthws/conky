include(ToLua)

if(BUILD_LUA_CAIRO)
	include_directories(${luacairo_includes} ${CMAKE_CURRENT_SOURCE_DIR})
	wrap_tolua(luacairo_src cairo.pkg)

	add_library(cairo SHARED ${luacairo_src})

	target_link_libraries(cairo ${luacairo_libs} ${TOLUA_LIBS})
	set(lua_libs ${lua_libs} cairo)
endif(BUILD_LUA_CAIRO)

if(BUILD_LUA_IMLIB2)
	include_directories(${luaimlib2_includes} ${CMAKE_CURRENT_SOURCE_DIR})
	wrap_tolua(luaimlib2_src imlib2.pkg)

	add_library(imlib2 SHARED ${luaimlib2_src})

	target_link_libraries(imlib2 ${luaimlib2_libs} ${TOLUA_LIBS})
	set(lua_libs ${lua_libs} imlib2)
endif(BUILD_LUA_IMLIB2)

install(TARGETS
	${lua_libs}
	LIBRARY DESTINATION lib/conky
	ARCHIVE DESTINATION lib/conky
)
