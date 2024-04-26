# Try to find the MPFR C++ (MPREAL) library
# See http://www.holoborodko.com/pavel/mpreal/
#
# This module supports requiring a minimum version, e.g. you can do
#   find_package(MPREAL 1.8.6)
# to require version 1.8.6 or newer of MPREAL C++.
#
# Once done this will define
#
#  MPREAL_FOUND - system has MPREAL lib with correct version
#  MPREAL_INCLUDES - MPREAL required include directories
#  MPREAL_LIBRARIES - MPREAL required libraries
#  MPREAL_VERSION - MPREAL version

# Copyright (c) 2020 The Eigen Authors.
# Redistribution and use is allowed according to the terms of the BSD license.

include(CMakeFindDependencyMacro)
find_dependency(MPFR)
find_dependency(GMP)

# Set MPREAL_INCLUDES
find_path(MPREAL_INCLUDES
  NAMES
  mpreal.h
  PATHS
  $ENV{GMPDIR}
  ${INCLUDE_INSTALL_DIR}
)

# Set MPREAL_FIND_VERSION to 1.0.0 if no minimum version is specified

if(NOT MPREAL_FIND_VERSION)
  if(NOT MPREAL_FIND_VERSION_MAJOR)
    set(MPREAL_FIND_VERSION_MAJOR 1)
  endif()
  if(NOT MPREAL_FIND_VERSION_MINOR)
    set(MPREAL_FIND_VERSION_MINOR 0)
  endif()
  if(NOT MPREAL_FIND_VERSION_PATCH)
    set(MPREAL_FIND_VERSION_PATCH 0)
  endif()

  set(MPREAL_FIND_VERSION "${MPREAL_FIND_VERSION_MAJOR}.${MPREAL_FIND_VERSION_MINOR}.${MPREAL_FIND_VERSION_PATCH}")
endif()

# Check bugs
# - https://github.com/advanpix/mpreal/issues/7
# - https://github.com/advanpix/mpreal/issues/9
set(MPREAL_TEST_PROGRAM "
#include <mpreal.h>
#include <algorithm>
int main(int argc, char** argv) {
  const mpfr::mpreal one  =    1.0;
  const mpfr::mpreal zero =    0.0;
  using namespace std;
  const mpfr::mpreal smaller = min(one, zero);
  return 0;
}")

if(MPREAL_INCLUDES)

  # Set MPREAL_VERSION
  
  file(READ "${MPREAL_INCLUDES}/mpreal.h" _mpreal_version_header)
  
  string(REGEX MATCH "define[ \t]+MPREAL_VERSION_MAJOR[ \t]+([0-9]+)" _mpreal_major_version_match "${_mpreal_version_header}")
  set(MPREAL_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+MPREAL_VERSION_MINOR[ \t]+([0-9]+)" _mpreal_minor_version_match "${_mpreal_version_header}")
  set(MPREAL_MINOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+MPREAL_VERSION_PATCHLEVEL[ \t]+([0-9]+)" _mpreal_patchlevel_version_match "${_mpreal_version_header}")
  set(MPREAL_PATCHLEVEL_VERSION "${CMAKE_MATCH_1}")
  
  set(MPREAL_VERSION ${MPREAL_MAJOR_VERSION}.${MPREAL_MINOR_VERSION}.${MPREAL_PATCHLEVEL_VERSION})
  
  # Check whether found version exceeds minimum version
  
  if(${MPREAL_VERSION} VERSION_LESS ${MPREAL_FIND_VERSION})
    set(MPREAL_VERSION_OK FALSE)
    message(STATUS "MPREAL version ${MPREAL_VERSION} found in ${MPREAL_INCLUDES}, "
                   "but at least version ${MPREAL_FIND_VERSION} is required")
  else()
    set(MPREAL_VERSION_OK TRUE)
    
    list(APPEND MPREAL_INCLUDES "${MPFR_INCLUDES}" "${GMP_INCLUDES}")
    list(REMOVE_DUPLICATES MPREAL_INCLUDES)
    
    list(APPEND MPREAL_LIBRARIES "${MPFR_LIBRARIES}" "${GMP_LIBRARIES}")
    list(REMOVE_DUPLICATES MPREAL_LIBRARIES)
    
    # Make sure it compiles with the current compiler.
    unset(MPREAL_WORKS CACHE)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES "${MPREAL_INCLUDES}")
    set(CMAKE_REQUIRED_LIBRARIES "${MPREAL_LIBRARIES}")
    check_cxx_source_compiles("${MPREAL_TEST_PROGRAM}" MPREAL_WORKS)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPREAL DEFAULT_MSG
                                  MPREAL_INCLUDES MPREAL_VERSION_OK MPREAL_WORKS)
mark_as_advanced(MPREAL_INCLUDES)
