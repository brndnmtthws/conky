# Find the native netlink includes and library
#
# If they exist, differentiate between versions 1, 2 and 3.
# Version 1 does not have netlink/version.h
# Version 2 started separating libraries (libnl{,-genl,-route}).
# Version 3 (>= 3.2) started appending the major version number as suffix to
# library names (libnl-3)
#
#  NL_INCLUDE_DIRS - where to find libnl.h, etc.
#  NL_LIBRARIES    - List of libraries when using libnl.
#  NL_FOUND        - True if libnl found.

# Based on wireshark FindNL:
# https://github.com/wireshark/wireshark/blob/master/cmake/modules/FindNL.cmake
# Modified so it doesn't make any hard requirements for libraries used by the
# project.

if(NL_LIBRARIES AND NL_INCLUDE_DIRS)
  # in cache already
  SET(NL_FOUND TRUE)
else()
  SET( SEARCHPATHS
      /opt/local
      /sw
      /usr
      /usr/local
  )

  find_package(PkgConfig)
  pkg_check_modules(NL3 libnl-3.0 libnl-genl-3.0 libnl-route-3.0)
  if(NOT NL3_FOUND)
    pkg_search_module(NL2 libnl-2.0)
    if (NL2_FOUND)
      mark_as_advanced(NL2_INCLUDE_DIR NL2_LIBRARY)
    endif()
  endif()

  # Try to find NL 2.0, 3.0 or 3.1 (/usr/include/netlink/version.h) or
  # NL >= 3.2 (/usr/include/libnl3/netlink/version.h)
  find_path(NL3_INCLUDE_DIR
    PATH_SUFFIXES
      include/libnl3
      include
    NAMES
      netlink/version.h
    HINTS
      "${NL3_libnl-3.0_includeDIR}"
      "${NL2_includeDIR}"
    PATHS
      $(SEARCHPATHS)
  )

  # NL version >= 2
  if(NL3_INCLUDE_DIR)
    find_library(NL3_LIBRARY
      NAMES
        nl-3 nl
      PATH_SUFFIXES
        lib64 lib
      HINTS
        "${NL3_libnl-3.0_LIBDIR}"
        "${NL2_LIBDIR}"
      PATHS
        $(SEARCHPATHS)
    )

    if(NL3_LIBRARY)
      set(NL_LIBRARIES ${NL_LIBRARIES} ${NL3_LIBRARY})
      set(NL_INCLUDE_DIRS ${NL_INCLUDE_DIRS} ${NL3_INCLUDE_DIR})
      set(HAVE_LIBNL 1)
      mark_as_advanced(NL3_LIBRARY HAVE_LIBNL)
    else()

    endif()

    find_library(NLGENL_LIBRARY
      NAMES
        nl-genl-3 nl-genl
      PATH_SUFFIXES
        lib64 lib
      HINTS
        "${NL3_libnl-genl-3.0_LIBDIR}"
        "${NL2_LIBDIR}"
      PATHS
        $(SEARCHPATHS)
    )

    if(NLGENL_LIBRARY)
      mark_as_advanced(NLGENL_LIBRARY)
      set(NL_LIBRARIES ${NL_LIBRARIES} ${NLGENL_LIBRARY})
      set(HAVE_LIBNL_GENL 1)
    endif()

    find_library(NLROUTE_LIBRARY
      NAMES
        nl-route-3 nl-route
      PATH_SUFFIXES
        lib64 lib
      HINTS
        "${NL3_libnl-route-3.0_LIBDIR}"
        "${NL2_LIBDIR}"
      PATHS
        $(SEARCHPATHS)
    )

    if(NLROUTE_LIBRARY)
      mark_as_advanced(NLROUTE_LIBRARY)
      set(NL_LIBRARIES ${NL_LIBRARIES} ${NLROUTE_LIBRARY})
      set(HAVE_LIBNL_ROUTE 1)
    endif()

    if(NOT NL3_FOUND AND NL2_FOUND)
      set(HAVE_LIBNL2 1)
    else()
      set(HAVE_LIBNL3 1)
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NL DEFAULT_MSG NL_LIBRARIES NL_INCLUDE_DIRS)
mark_as_advanced(NL_LIBRARIES NL_INCLUDE_DIRS)

