# LuaDist CMake utility library.
# Provides variables and utility functions common to LuaDist CMake builds.
# 
# Copyright (C) 2007-2010 LuaDist.
# by David Manura, Peter Drahos
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

# Few convinence settings
SET (CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)
SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

# Where to install module parts:
set(INSTALL_BIN bin CACHE PATH "Where to install binaries to.")
set(INSTALL_LIB lib CACHE PATH "Where to install libraries to.")
set(INSTALL_INC include CACHE PATH "Where to install headers to.")
set(INSTALL_ETC etc CACHE PATH "Where to store configuration files")
set(INSTALL_LMOD share/lua/lmod CACHE PATH "Directory to install Lua modules.")
set(INSTALL_CMOD share/lua/cmod CACHE PATH "Directory to install Lua binary modules.")
set(INSTALL_DATA share/${PROJECT_NAME} CACHE PATH "Directory the package can store documentation, tests or other data in.")
set(INSTALL_DOC ${INSTALL_DATA}/doc CACHE PATH "Recommended directory to install documentation into.")
set(INSTALL_EXAMPLE ${INSTALL_DATA}/example CACHE PATH "Recommended directory to install examples into.")
set(INSTALL_TEST ${INSTALL_DATA}/test CACHE PATH "Recommended directory to install tests into.")
set(INSTALL_FOO ${INSTALL_DATA}/etc CACHE PATH "Where to install additional files")


# In MSVC, prevent warnings that can occur when using standard libraries.
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

# Adds Lua shared library module target `_target`.
# Additional sources to build the module are listed after `_target`.
macro(add_lua_module _target)
	find_package(Lua51 REQUIRED)
	include_directories(${LUA_INCLUDE_DIR})  #2DO: somehow apply only to _target?

	add_library(${_target} MODULE ${ARGN})
	set_target_properties(${_target} PROPERTIES PREFIX "")
	target_link_libraries(${_target} ${LUA_LIBRARY})
	
	IF(WIN32)
		set_target_properties(${_target} PROPERTIES LINK_FLAGS "-Wl,--enable-auto-import")
	ENDIF()

endmacro(add_lua_module)

# Runs Lua script `_testfile` under CTest tester.
# Optional argument `_testcurrentdir` is current working directory to run test under
# (defaults to ${CMAKE_CURRENT_BINARY_DIR}).
# Both paths, if relative, are relative to ${CMAKE_CURRENT_SOURCE_DIR}.
# Under LuaDist, set test=true in config.lua to enable testing.
macro(add_lua_test _testfile)
	include(CTest)
	if(BUILD_TESTING)
		find_program(LUA NAMES lua lua.bat)
		get_filename_component(TESTFILEABS ${_testfile} ABSOLUTE)
		get_filename_component(TESTFILENAME ${_testfile} NAME)
		get_filename_component(TESTFILEBASE ${_testfile} NAME_WE)

		# Write wrapper script.
		set(TESTWRAPPER ${CMAKE_CURRENT_BINARY_DIR}/${TESTFILENAME})
		set(TESTWRAPPERSOURCE
"package.path = '${CMAKE_CURRENT_BINARY_DIR}/?.lua\;${CMAKE_CURRENT_SOURCE_DIR}/?.lua\;' .. package.path
package.cpath = '${CMAKE_CURRENT_BINARY_DIR}/?.so\;${CMAKE_CURRENT_BINARY_DIR}/?.dll\;' .. package.cpath
return dofile '${TESTFILEABS}'
"		)
		if(${ARGC} GREATER 1)
			set(_testcurrentdir ${ARGV1})
			get_filename_component(TESTCURRENTDIRABS ${_testcurrentdir} ABSOLUTE)
			set(TESTWRAPPERSOURCE
"require 'lfs'
lfs.chdir('${TESTCURRENTDIRABS}')
${TESTWRAPPERSOURCE}")
		endif()
		FILE(WRITE ${TESTWRAPPER} ${TESTWRAPPERSOURCE})

		add_test(${TESTFILEBASE} ${LUA} ${TESTWRAPPER})
	endif(BUILD_TESTING)

	# see also http://gdcm.svn.sourceforge.net/viewvc/gdcm/Sandbox/CMakeModules/UsePythonTest.cmake
endmacro(add_lua_test)

# Converts Lua source file `_source` to binary string embedded in C source
# file `_target`.  Optionally compiles Lua source to byte code (not available
# under LuaJIT2, which doesn't have a bytecode loader).  Additionally, Lua
# versions of bin2c [1] and luac [2] may be passed respectively as additional
# arguments.
#
# [1] http://lua-users.org/wiki/BinToCee
# [2] http://lua-users.org/wiki/LuaCompilerInLua
function(add_lua_bin2c _target _source)
	find_program(LUA NAMES lua lua.bat)
	execute_process(COMMAND ${LUA} -e "string.dump(function()end)" RESULT_VARIABLE _LUA_DUMP_RESULT ERROR_QUIET)
	if (NOT ${_LUA_DUMP_RESULT})
		SET(HAVE_LUA_DUMP true)
	endif()
	message("-- string.dump=${HAVE_LUA_DUMP}")

	if (ARGV2)
		get_filename_component(BIN2C ${ARGV2} ABSOLUTE)
		set(BIN2C ${LUA} ${BIN2C})
	else()
		find_program(BIN2C NAMES bin2c bin2c.bat)
	endif()
	if (HAVE_LUA_DUMP)
		if (ARGV3)
			get_filename_component(LUAC ${ARGV3} ABSOLUTE)
			set(LUAC ${LUA} ${LUAC})
		else()
			find_program(LUAC NAMES luac luac.bat)
		endif()
	endif (HAVE_LUA_DUMP)
	message("-- bin2c=${BIN2C}")
	message("-- luac=${LUAC}")

	get_filename_component(SOURCEABS ${_source} ABSOLUTE)
	if (HAVE_LUA_DUMP)
		get_filename_component(SOURCEBASE ${_source} NAME_WE)
		add_custom_command(
			OUTPUT  ${_target} DEPENDS ${_source}
			COMMAND ${LUAC} -o ${CMAKE_CURRENT_BINARY_DIR}/${SOURCEBASE}.lo ${SOURCEABS}
			COMMAND ${BIN2C} ${CMAKE_CURRENT_BINARY_DIR}/${SOURCEBASE}.lo ">${_target}" )
	else()
		add_custom_command(
			OUTPUT  ${_target} DEPENDS ${SOURCEABS}
			COMMAND ${BIN2C} ${_source} ">${_target}" )
	endif()
endfunction(add_lua_bin2c)
