# Distributed under the MIT/Apache 2.0/zlib License.
# Author: Tin Švagelj (Caellian) <tin.svagelj@live.com>

#[[.md:
# TargetPlatformTests

This module is a replacement for standard platform checking macros provided by
CMake:
- `CHECK_INCLUDE_FILES`
- `CHECK_FUNCTION_EXISTS`
- `CHECK_SYMBOL_EXISTS`
- `CHECK_LIBRARY_EXISTS`

The provided alternatives call a `TPT_DEFINE_CB` callback if it's provided (set
to a function/macro name that accepts one argument).
Example:
```cmake
macro(source_define DEFINE_NAME)
    set(VALUE "")
    if(ARGC GREATER 1)
        set(VALUE "=${ARGV1}")
    endif()
    target_compile_definitions(my_program INTERFACE "${DEFINE_NAME}${VALUE}")
endmacro()

set(TPT_DEFINE_CB source_define)
```

Unlike original functions, provided alteratives should be assumed to ignore
globals and are exclusively bound to the one that's specified for target(s)
provided via `ADD_TESTING_TARGET(target)` macro. This must be cleared with
`CLEAR_TESTING_TARGETS()` macro after checking is finished to reset the
checking environment and revert globals to their original state.

It's near impossible to re-construct globals to their initial (compiler
dependant) values once they've been mutated. So if global include/link paths
have been modified or custom compiler options have been added to arguments,
these macros will be forced use those.
]]

# include(CheckFunctionExists) # It's not good enough
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckLibraryExists)
include(CheckSourceCompiles)

# A list of ambient variables which should be temporarily cleared when doing
# platform checks.
# See: https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html
set(_TPT_INTERNAL_AMBIENT_VARIABLES
    CMAKE_REQUIRED_FLAGS
    CMAKE_REQUIRED_DEFINITIONS
    CMAKE_REQUIRED_INCLUDES
    CMAKE_REQUIRED_LINK_OPTIONS
    CMAKE_REQUIRED_LIBRARIES
    CMAKE_REQUIRED_LINK_DIRECTORIES
    CMAKE_REQUIRED_QUIET
    # Statefull globals like INCLUDE_DIRECTORIES and similiar are ignored.
    # They will stay polluted until the end of the build.
    # Generally, to avoid weird side-effects and stateful builds, one should
    # only modify targets.
)
set(_TPT_INTERNAL_DEFAULT_LANGUAGE CXX)

# INTERNAL macro for ADD_TESTING_TARGET, do not use directly because it's
# missing checks.
macro(_TPT_INTERNAL_DO_ADD_TESTING_TARGET FROM)
    # Retrieve and append target properties of FROM
    #[[ Full list (as of 3.31.3) includes:
        INTERFACE_AUTOMOC_MACRO_NAMES           # IRELLEVANT
        INTERFACE_AUTOUIC_OPTIONS               # IRELLEVANT uic stuff
        INTERFACE_COMPILE_DEFINITIONS           # USED
        INTERFACE_COMPILE_FEATURES              # MAPPED
        INTERFACE_COMPILE_OPTIONS               # USED
        INTERFACE_CXX_MODULE_SETS               # NOT USED modules
        INTERFACE_HEADER_SETS                   # NOT USED modules
        INTERFACE_HEADER_SETS_TO_VERIFY         # NOT USED modules
        INTERFACE_INCLUDE_DIRECTORIES           # USED
        INTERFACE_LINK_DEPENDS                  # NOT USED linker scripts
        INTERFACE_LINK_DIRECTORIES              # USED
        INTERFACE_LINK_LIBRARIES                # USED
        INTERFACE_LINK_LIBRARIES_DIRECT         # IRELLEVANT not a dependency
        INTERFACE_LINK_LIBRARIES_DIRECT_EXCLUDE # IRELLEVANT not a dependency
        INTERFACE_LINK_OPTIONS                  # USED
        INTERFACE_POSITION_INDEPENDENT_CODE     # IRELLEVANT not a dependency
        INTERFACE_PRECOMPILE_HEADERS            # IRELLEVANT not a dependency
        INTERFACE_SOURCES                       # IRELLEVANT not a dependency
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES    # IRELLEVANT no thirdparty dependencies
    ]]
    get_target_property(_TPT_INTERNAL_COMPILE_DEFINITIONS ${FROM} INTERFACE_COMPILE_DEFINITIONS)
    get_target_property(_TPT_INTERNAL_COMPILE_FEATURES ${FROM} INTERFACE_COMPILE_FEATURES)
    get_target_property(_TPT_INTERNAL_COMPILE_OPTIONS ${FROM} INTERFACE_COMPILE_OPTIONS)
    get_target_property(_TPT_INTERNAL_INCLUDE_DIRECTORIES ${FROM} INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(_TPT_INTERNAL_LINK_OPTIONS ${FROM} INTERFACE_LINK_OPTIONS)
    get_target_property(_TPT_INTERNAL_LINK_DIRECTORIES ${FROM} INTERFACE_LINK_DIRECTORIES)
    get_target_property(_TPT_INTERNAL_TEST_LIBRARIES ${FROM} INTERFACE_LINK_LIBRARIES)

    set(_TPT_INTERNAL_LIBRARIES)
    set(_TPT_INTERNAL_TARGETS)
    foreach(LIBRARY IN LISTS _TPT_INTERNAL_TEST_LIBRARIES)
        if(TARGET ${LIBRARY})
            list(APPEND _TPT_INTERNAL_TARGETS "${LIBRARY}")
        else()
            list(APPEND _TPT_INTERNAL_LIBRARIES "${LIBRARY}")
        endif()
    endforeach()

    if(_TPT_INTERNAL_COMPILE_FEATURES)
        foreach(feature IN LISTS _TPT_INTERNAL_COMPILE_FEATURES)
            if(feature STREQUAL cxx_std_11)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE CXX)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -std=c++11")
            elseif(feature STREQUAL cxx_std_14)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE CXX)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -std=c++14")
            elseif(feature STREQUAL cxx_std_17)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE CXX)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -std=c++17")
            elseif(feature STREQUAL cxx_std_20)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE CXX)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -std=c++20")
            elseif(feature STREQUAL cxx_std_23)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE CXX)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -std=c++23")
            elseif(feature STREQUAL c_std_90)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE C)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_C "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_C} -std=c90")
            elseif(feature STREQUAL c_std_99)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE C)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_C "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_C} -std=c99")
            elseif(feature STREQUAL c_std_11)
                set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE C)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_C "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_C} -std=c11")
            elseif(feature STREQUAL cxx_variadic_templates)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -fvariadic-templates")
            elseif(feature STREQUAL cxx_range_for)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -frange-for")
            elseif(feature STREQUAL cxx_rvalue_references)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -frvalue-references")
            elseif(feature STREQUAL cxx_static_assert)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -fstatic-assert")
            elseif(feature STREQUAL cxx_constexpr)
                set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX} -fconstexpr")
            else()
                # Many more.
                message(WARNING "Feature to flag mapping not handled for ${feature}")
            endif()
        endforeach()
    endif()
    if(_TPT_INTERNAL_COMPILE_OPTIONS)
        set(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_COMMON ${_TPT_INTERNAL_COMPILE_OPTIONS})
    endif()
    if(_TPT_INTERNAL_COMPILE_DEFINITIONS)
        foreach(DEFINITION IN LISTS _TPT_INTERNAL_COMPILE_DEFINITIONS)
            list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D${DEFINITION}")
        endforeach()
    endif()
    if(_TPT_INTERNAL_INCLUDE_DIRECTORIES)
        list(APPEND CMAKE_REQUIRED_INCLUDES ${_TPT_INTERNAL_INCLUDE_DIRECTORIES})
    endif()
    if(_TPT_INTERNAL_LINK_OPTIONS)
        list(APPEND CMAKE_REQUIRED_LINK_OPTIONS ${_TPT_INTERNAL_LINK_OPTIONS})
    endif()
    if(_TPT_INTERNAL_LIBRARIES)
        list(APPEND CMAKE_REQUIRED_LIBRARIES ${_TPT_INTERNAL_LIBRARIES})
    endif()
    if(_TPT_INTERNAL_LINK_DIRECTORIES)
        list(APPEND CMAKE_REQUIRED_LINK_DIRECTORIES ${_TPT_INTERNAL_LINK_DIRECTORIES})
    endif()

    # Recursively add any additional target flags
    foreach(TARGET IN LISTS _TPT_INTERNAL_TARGETS)
        if(${TARGET} IN_LIST _TPT_INTERNAL_VISITED_DEPENDENCY_TARGETS)
            continue()
        endif()
        _tpt_internal_do_add_testing_target(${TARGET})
        list(APPEND _TPT_INTERNAL_VISITED_DEPENDENCY_TARGETS "${TARGET}")
    endforeach()
endmacro()

# Internal utility macros for affecting all ambient variables in bulk
macro(_TPT_INTERNAL_CLEAR_AMBIENT VARIABLE)
    if(DEFINED ${VARIABLE})
        set("_TPT_INTERNAL_INITIAL_${VARIABLE}_VALUE" ${VARIABLE})
        unset(${VARIABLE})
    endif()
endmacro()
macro(_TPT_INTERNAL_RESET_AMBIENT VARIABLE)
    if(DEFINED "_TPT_INTERNAL_INITIAL_${VARIABLE}_VALUE")
        set(${VARIABLE} "_TPT_INTERNAL_INITIAL_${VARIABLE}_VALUE")
        unset("_TPT_INTERNAL_INITIAL_${VARIABLE}_VALUE")
    elseif(DEFINED ${VARIABLE})
        unset(${VARIABLE})
    endif()
endmacro()
macro(_TPT_INTERNAL_FOR_ALL_AMBIENT ACTION)
    foreach(VARIABLE IN LISTS _TPT_INTERNAL_AMBIENT_VARIABLES)
        cmake_language(EVAL CODE "${ACTION}(${VARIABLE})")
    endforeach()
endmacro()

macro(_TPT_INTERNAL_SETUP_RUN_ENV LANGUAGE)
    set(CMAKE_REQUIRED_FLAGS "${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_COMMON} ${_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_${LANGUAGE}}")
endmacro()
macro(_TPT_INTERNAL_DO_DEFINE_FOR_SOURCE DEFINE_NAME)
    if(DEFINED TPT_DEFINE_CB)
        cmake_language(EVAL CODE "${TPT_DEFINE_CB}(${DEFINE_NAME})")
    endif()
endmacro()

# A function signature must be formatted like:
# OUT_TYPE FN_NAME(ARGUMENTS)
# Without any attributes (constexpr, inline, noexcept, etc.)
macro(_TPT_INTERNAL_FN_SIG_SPLIT SIGNATURE TYPE_OUT NAME_OUT ARGS_OUT)
    string(STRIP "${SIGNATURE}" SIGNATURE)
    string(REGEX MATCH "[a-zA-Z_][a-zA-Z_0-9]*(::[a-zA-Z_][a-zA-Z_0-9]*)*\\(" ${NAME_OUT} "${SIGNATURE}")
    string(LENGTH "${${NAME_OUT}}" _TPT_FN_NAME)
    if(NOT ${_TPT_FN_NAME})
        message(FATAL_ERROR "Invalid function name specified in signature '${SIGNATURE}'!")
    endif()
    math(EXPR _TPT_FN_NAME "${_TPT_FN_NAME}-1")
    string(SUBSTRING "${${NAME_OUT}}" 0 ${_TPT_FN_NAME} ${NAME_OUT})
    unset(_TPT_FN_NAME)
    string(STRIP "${${NAME_OUT}}" ${NAME_OUT})

    string(FIND "${SIGNATURE}" "${${NAME_OUT}}" _TPT_FN_NAME_START)
    math(EXPR _TPT_FN_NAME_START "${_TPT_FN_NAME_START}")
    string(SUBSTRING "${SIGNATURE}" 0 ${_TPT_FN_NAME_START} ${TYPE_OUT})
    unset(_TPT_FN_NAME_START)
    string(STRIP "${${TYPE_OUT}}" ${TYPE_OUT})

    string(REGEX MATCH "\\([^)]*\\)$" ${ARGS_OUT} "${SIGNATURE}")
    string(LENGTH "${${ARGS_OUT}}" _TPT_FN_ARGS_LEN)
    if(NOT ${_TPT_FN_ARGS_LEN})
        message(FATAL_ERROR "Function signature '${SIGNATURE}' is missing arguments!")
    endif()
    math(EXPR _TPT_FN_ARGS_LEN "${_TPT_FN_ARGS_LEN}-2")
    string(SUBSTRING "${${ARGS_OUT}}" 1 ${_TPT_FN_ARGS_LEN} ${ARGS_OUT})
    unset(_TPT_FN_ARGS_LEN)
    string(STRIP "${${ARGS_OUT}}" ${ARGS_OUT})
endmacro()

# Copy any definitions from provided target. If called multiple times, using
# CLEAR_TESTING_TARGETS will reset globals to their value during first
# invocation.
macro(ADD_TESTING_TARGET FROM)
    if(NOT _TPT_INTERNAL_CHECK_ENVIRONEMENT_SET)
        _tpt_internal_for_all_ambient(_tpt_internal_clear_ambient)
    endif()

    set(_TPT_INTERNAL_VISITED_DEPENDENCY_TARGETS)
    _tpt_internal_do_add_testing_target(${FROM})

    if(NOT _TPT_INTERNAL_TEST_CUSTOM_LANGUAGE)
        set(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE "${_TPT_INTERNAL_DEFAULT_LANGUAGE}")
    endif()

    set(CMAKE_REQUIRED_QUIET TRUE)
    set(_TPT_INTERNAL_CHECK_ENVIRONEMENT_SET TRUE)
endmacro()

#[[.md:
TEST_INCLUDES checks whether one or more headers can be included from
currently active testing targets.

```cmake
test_includes("<header>[;<header>...]" <variable> [LANGUAGE <language>] [REQUIRED])
```

Much like CHECK_INCLUDE_FILES, this check ensures one of the include
directories for tested targets actually provides a header that will be needed
by the program.

This is a variant of CHECK_INCLUDE_FILES that works with targets provided to
ADD_TESTING_TARGET, instead of requiring all the flags to be set manually.

See [CheckIncludeFiles](https://cmake.org/cmake/help/latest/module/CheckIncludeFiles.html) for details.
]]
function(test_includes INCLUDE VARIABLE)
    if(NOT _TPT_INTERNAL_CHECK_ENVIRONEMENT_SET)
        message(FATAL_ERROR "Environment not configured; Use ADD_TESTING_TARGET(target) macro!")
    endif()
    cmake_parse_arguments(PARSE_ARGV 2 arg
        "REQUIRED" "LANGUAGE" ""
    )
    if(NOT DEFINED arg_LANGUAGE)
        set(arg_LANGUAGE ${_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE})
    endif()
    _tpt_internal_setup_run_env(${arg_LANGUAGE})
    unset("${VARIABLE}" CACHE)
    check_include_files("${INCLUDE}" "${VARIABLE}" LANGUAGE ${arg_LANGUAGE})
    if("${${VARIABLE}}")
        if(NOT arg_REQUIRED)
            _tpt_internal_do_define_for_source(${VARIABLE})
        endif()
        message(STATUS "'${INCLUDE}' include - found")
        set(${VARIABLE} 1 CACHE INTERNAL "Have include ${INCLUDE}")
    else()
        if(arg_REQUIRED)
            message(FATAL_ERROR "'${INCLUDE}' include - not found")
        else()
            message(STATUS "'${INCLUDE}' include - not found")
        endif()
        set(${VARIABLE} 0 CACHE INTERNAL "Have include ${INCLUDE}")
    endif()
endfunction()

#[[.md:
TEST_FUNCTION checks whether a function with provided signature exists (when
some headers are included).

```cmake
test_function("<header>[;<header>...]" <function-signature> <variable> [LANGUAGE <language>] [REQUIRED])
```

`function-signature` should not contain any attributes as those will be ignored.

This function is different from standard CHECK_FUNCTION_EXISTS macro because
the standard macro only checks whether a function can be linked from available
libraries. This version also checks the function signature.

This variant also works with targets provided to ADD_TESTING_TARGET, instead
of requiring all the flags to be set manually.
]]
function(test_function HEADERS FUNCTION_SIGNATURE VARIABLE)
    if(NOT _TPT_INTERNAL_CHECK_ENVIRONEMENT_SET)
        message(FATAL_ERROR "Environment not configured; Use ADD_TESTING_TARGET(target) macro!")
    endif()

    cmake_parse_arguments(PARSE_ARGV 3 arg
        "REQUIRED" "LANGUAGE" ""
    )
    if(NOT DEFINED arg_LANGUAGE)
        set(arg_LANGUAGE ${_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE})
    endif()
    _tpt_internal_fn_sig_split("${FUNCTION_SIGNATURE}" FUNCTION_RETURN FUNCTION_NAME FUNCTION_ARGUMENTS)
    _tpt_internal_setup_run_env(${arg_LANGUAGE})

    set(_TPT_INTERNAL_HEADER_LIST "")
    if(NOT HEADERS STREQUAL "")
        foreach(HEADER IN LISTS HEADERS)
            set(_TPT_INTERNAL_HEADER_LIST "${_TPT_INTERNAL_HEADER_LIST}\n#include <${HEADER}>")
        endforeach()
    endif()

    unset("${VARIABLE}" CACHE)
    if(arg_LANGUAGE STREQUAL "C")
        check_source_compiles(C
        "#include <stddef.h>
        #include <stdlib.h>${_TPT_INTERNAL_HEADER_LIST}
        int main(void) {
            ${FUNCTION_RETURN} (*_function_name_test)(${FUNCTION_ARGUMENTS});
            _function_name_test = &${FUNCTION_NAME};
            return _function_name_test != NULL; // use the pointer
        }" "${VARIABLE}")
    elseif(arg_LANGUAGE STREQUAL "CXX")
        check_source_compiles(CXX
        "#include <cstddef>
        #include <cstdlib>
        #include <type_traits>${_TPT_INTERNAL_HEADER_LIST}
        #if __cplusplus >= 201703L
        template <typename Fn, typename... Args> // C++17 and later
        using result_type_t = std::result_of_t<Fn(Args...)>;
        #else
        template <typename Fn, typename... Args> // C++11 and C++14
        using result_type_t = typename std::result_of<Fn(Args...)>::type;
        #endif
        #pragma GCC diagnostic ignored \"-Wignored-attributes\"
        #pragma clang diagnostic ignored \"-Wignored-attributes\"
        int main() {
          static_assert(std::is_same<
            result_type_t<std::decay<decltype(&${FUNCTION_NAME})>::type, ${FUNCTION_ARGUMENTS}>,
            ${FUNCTION_RETURN}
          >::value, \"strndup is not a function with expected signature\");
          return 0;
        }" "${VARIABLE}")
    else()
        message(FATAL_ERROR "${arg_LANGUAGE} not supported by TEST_FUNCTION")
    endif()
    if("${${VARIABLE}}")
        message(STATUS "'${FUNCTION_NAME}' function - found")
        if(NOT arg_REQUIRED)
            _tpt_internal_do_define_for_source("${VARIABLE}")
        endif()
        set(${VARIABLE} 1 CACHE INTERNAL "Have function ${FUNCTION_NAME}")
    else()
        if(arg_REQUIRED)
            message("Checked function signature: ${FUNCTION_SIGNATURE}")
            message(FATAL_ERROR "'${FUNCTION_NAME}' function - not found")
        else()
            message(STATUS "'${FUNCTION_NAME}' function - not found")
        endif()
        set(${VARIABLE} 0 CACHE INTERNAL "Have function ${FUNCTION_NAME}")
    endif()
endfunction()

#[[.md:
TEST_SYMBOL checks whether `<header>`s provide some `<symbol>` needed by the
program.

```cmake
test_symbol("<header>[;<header>...]" <symbol> <variable> [LANGUAGE <language>] [REQUIRED])
```

This is a variant of CHECK_SYMBOL_EXISTS that works with targets provided to
ADD_TESTING_TARGET, instead of requiring all the flags to be set manually.

See [CheckSymbolExists](https://cmake.org/cmake/help/latest/module/CheckSymbolExists.html) for details.
]]
function(test_symbol HEADER SYMBOL VARIABLE)
    if(NOT _TPT_INTERNAL_CHECK_ENVIRONEMENT_SET)
        message(FATAL_ERROR "Environment not configured; Use ADD_TESTING_TARGET(target) macro!")
    endif()

    cmake_parse_arguments(PARSE_ARGV 3 arg
        "REQUIRED" "LANGUAGE" ""
    )
    if(NOT DEFINED arg_LANGUAGE)
        set(arg_LANGUAGE ${_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE})
    endif()

    unset("${VARIABLE}" CACHE)
    if(arg_LANGUAGE STREQUAL "C")
        check_symbol_exists("${SYMBOL}" "${HEADER}" "${VARIABLE}")
    elseif(arg_LANGUAGE STREQUAL "CXX")
        check_cxx_symbol_exists("${SYMBOL}" "${HEADER}" "${VARIABLE}")
    else()
        message(FATAL_ERROR "${LANGUAGE} not supported by TEST_SYMBOL")
    endif()
    
    _tpt_internal_setup_run_env(${arg_LANGUAGE})
    if("${${VARIABLE}}")
        message(STATUS "'${SYMBOL}' symbol - found")
        if(NOT arg_REQUIRED)
            _tpt_internal_do_define_for_source("${VARIABLE}")
        endif()
        set(${VARIABLE} 1 CACHE INTERNAL "Have symbol ${SYMBOL}")
    else()
        if(arg_REQUIRED)
            message(FATAL_ERROR "'${SYMBOL}' symbol - not found")
        else()
            message(STATUS "'${SYMBOL}' symbol - not found")
        endif()
        set(${VARIABLE} 0 CACHE INTERNAL "Have symbol ${SYMBOL}")
    endif()
endfunction()

#[[.md:
TEST_LIBRARY checks whether a `<library>` from specific `<location>` provides
some `<function-name>` needed by the program.

```cmake
test_library(<library> "<function-name>" "[<location>]" <variable> [REQUIRED])
```

This is a variant of CHECK_LIBRARY_EXISTS that works with targets provided to
ADD_TESTING_TARGET, instead of requiring all the flags to be set manually.

See [CheckLibraryExists](https://cmake.org/cmake/help/latest/module/CheckLibraryExists.html) for details.
]]
function(test_library LIBRARY FUNCTION LOCATION VARIABLE)
    if(NOT _TPT_INTERNAL_CHECK_ENVIRONEMENT_SET)
        message(FATAL_ERROR "Environment not configured; Use ADD_TESTING_TARGET(target) macro!")
    endif()
    cmake_parse_arguments(PARSE_ARGV 4 arg
        "REQUIRED" "" ""
    )
    _tpt_internal_setup_run_env(${_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE})
    unset("${VARIABLE}" CACHE)
    check_library_exists("${LIBRARY}" "${FUNCTION}" "${LOCATION}" "${VARIABLE}")
    if("${${VARIABLE}}")
        message(STATUS "'${LIBRARY}' library function '${FUNCTION}' - found")
        if(NOT arg_REQUIRED)
            _tpt_internal_do_define_for_source("${VARIABLE}")
        endif()
        set(${VARIABLE} 1 CACHE INTERNAL "Have library ${LIBRARY}")
    else()
        if(arg_REQUIRED)
            message(FATAL_ERROR "'${LIBRARY}' library function '${FUNCTION}' - not found")
        else()
            message(STATUS "'${LIBRARY}' library function '${FUNCTION}' - not found")
        endif()
        set(${VARIABLE} 0 CACHE INTERNAL "Have library ${LIBRARY}")
    endif()
endfunction()

macro(CLEAR_TESTING_TARGETS)
    _TPT_INTERNAL_for_all_ambient(_TPT_INTERNAL_reset_ambient)
    unset(_TPT_INTERNAL_TEST_CUSTOM_LANGUAGE)
    unset(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_COMMON)
    unset(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_C)
    unset(_TPT_INTERNAL_CMAKE_REQUIRED_FLAGS_CXX)
    unset(_TPT_INTERNAL_CHECK_ENVIRONEMENT_SET)
endmacro()
