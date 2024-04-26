find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    option(INIT_SUBMODULES "Check submodules during build" ON)
    if(INIT_SUBMODULES)
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(GIT_SUBMOD_RESULT EQUAL "0")
            message(STATUS "Submodules initialized")
        else()
            message(FATAL_ERROR "Unable to initalize git submodules; error: ${GIT_SUBMOD_RESULT}")
        endif()
    endif()
endif()
