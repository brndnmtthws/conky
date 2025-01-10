set(PACKAGE_NAME "conky" CACHE INTERNAL "Name of the package")

# Current version
set(VERSION_MAJOR "1" CACHE INTERNAL "${PACKAGE_NAME} major version")
set(VERSION_MINOR "22" CACHE INTERNAL "${PACKAGE_NAME} minor version")
set(VERSION_PATCH "1" CACHE INTERNAL "${PACKAGE_NAME} patch version")

find_program(APP_AWK awk)

if(NOT APP_AWK)
  message(FATAL_ERROR "Unable to find program 'awk'")
endif(NOT APP_AWK)

find_program(APP_WC wc)

if(NOT APP_WC)
  message(FATAL_ERROR "Unable to find program 'wc'")
endif(NOT APP_WC)

find_program(APP_UNAME uname)

if(NOT APP_UNAME)
  message(FATAL_ERROR "Unable to find program 'uname'")
endif(NOT APP_UNAME)

if(NOT RELEASE)
  find_program(APP_GIT git)

  if(NOT APP_GIT)
    message(FATAL_ERROR "Unable to find program 'git'")
  endif(NOT APP_GIT)

  mark_as_advanced(APP_GIT)
endif(NOT RELEASE)

mark_as_advanced(APP_AWK APP_WC APP_UNAME)

execute_process(COMMAND ${APP_UNAME} -sm
  RESULT_VARIABLE RETVAL
  OUTPUT_VARIABLE BUILD_ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND ${APP_GIT} rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  RESULT_VARIABLE RETVAL
  OUTPUT_VARIABLE GIT_SHORT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(RELEASE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" CACHE INTERNAL "${PACKAGE_NAME} release version")

if(RELEASE)
  set(VERSION ${RELEASE_VERSION})
else(RELEASE)
  set(VERSION
    "${RELEASE_VERSION}-pre-${GIT_SHORT_SHA}")
endif(RELEASE)

set(COPYRIGHT "Copyright Brenden Matthews, et al, 2005-2024" CACHE INTERNAL "${PACKAGE_NAME} copyright")

# A function to print the target build properties
function(print_target_properties tgt)
  if(NOT TARGET ${tgt})
    message("There is no target named '${tgt}'")
    return()
  endif()

  # this list of properties can be extended as needed
  set(CMAKE_PROPERTY_LIST
    SOURCE_DIR
    BINARY_DIR
    COMPILE_DEFINITIONS
    COMPILE_OPTIONS
    INCLUDE_DIRECTORIES
    LINK_LIBRARIES)

  message("Configuration for target ${tgt}")

  foreach(prop ${CMAKE_PROPERTY_LIST})
    get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)

    if(propval)
      get_target_property(propval ${tgt} ${prop})
      message(STATUS "${tgt} ${prop} = ${propval}")
    endif()
  endforeach(prop)
endfunction(print_target_properties)

#[[
Conky uses a C++ compiler to compile the sources.

DO NOT add c_std_99, because it will cause warnings, which are errors in
MAINTANER_MODE, and that will cause all platform tests to fail in the CI.
c_std_99 only causes a warning with g++/clang++, because they ARE NOT C
compilers. Conky can't be compiled with a C compiler because it's a C++
program. A program can't simutaneously adhere to both C and C++
specifications because they're fundamentally different.
]]
add_library(conky-options INTERFACE)
target_compile_features(conky-options INTERFACE cxx_std_17)
target_compile_options(conky-options INTERFACE -Wpedantic) # disable compiler extensions

# In 99% of use cases this shouldn't be touched. However, if some standard implementation for
# compilation target is bad, it might be necessary to override the defaults.
# Value is a ';'-delimited list of custom compilation definitions (without a '-D' prefix, like defaults).
option(CUSTOM_COMPILE_DEFINITIONS "" "")
mark_as_advanced(CUSTOM_COMPILE_DEFINITIONS)
if(NOT CUSTOM_COMPILE_DEFINITIONS)
  # Feature configuration
  # These are shared across all supported platforms to ensure build consistency.
  target_compile_definitions(conky-options INTERFACE
    _POSIX_C_SOURCE=200809L # Use POSIX.1-2008     - https://ieeexplore.ieee.org/document/4694976
    # _XOPEN_SOURCE=700     # Use X/Open 7 / SUSv4 - https://pubs.opengroup.org/onlinepubs/9699919799/
    _FILE_OFFSET_BITS=64    # Use 64-bit types
    _LARGEFILE64_SOURCE     # Enable statfs64
    # _GNU_SOURCE is intentionally excluded to disable non-standard GNU extensions
  )
else()
  foreach(ITEM IN LISTS CUSTOM_COMPILE_DEFINITIONS)
    target_compile_definitions(conky-options INTERFACE ${ITEM})
  endforeach()
endif()

if(NOT OS_OPENBSD)
  # Always use libc++ when compiling w/ clang
  # Not on OpenBSD because that's the default for its vendored Clang (warning is error -> build fails)
  target_compile_options(conky-options INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-stdlib=libc++>
  )
  target_link_options(conky-options INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-stdlib=libc++>
  )
endif()

target_compile_options(conky-options INTERFACE
  $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-unknown-warning-option>
  $<$<COMPILE_LANG_AND_ID:CXX,GCC>:-Wno-unknown-warning>
)

#[[
Provides an interface to store compile-time definitions in. This is similar to
configuring a header, only it doesn't require copying all variables multiple
times (in CMake and in header input) and including that header in every place
it's being used.

This shouldn't contain any defines that affect third-party dependencies
(e.g. _POSIX_C_SOURCE). Those go into `conky-definitions` target.
]]
add_library(conky-definitions INTERFACE)

# Utility function that adds a source #define to default compilation targets.
# This replaces having to configure and import a header file.
function(source_define DEFINE_NAME)
    set(VALUE "")
    if(ARGC GREATER 1)
        set(VALUE "=${ARGV1}")
    endif()
    target_compile_definitions(conky-definitions INTERFACE "${DEFINE_NAME}${VALUE}")
endfunction()

source_define(SYSTEM_NAME "\"${CMAKE_SYSTEM_NAME}\"")
source_define(PACKAGE_NAME "\"${PACKAGE_NAME}\"")
source_define(VERSION "\"${VERSION}\"")
source_define(BUILD_ARCH "\"${BUILD_ARCH}\"")
