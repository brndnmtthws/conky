# * Find the Xinerama include file and library
#

set(Xinerama_INC_SEARCH_PATH
    /usr/X11R6/include
    /usr/local/include
    /usr/include/X11
    /usr/openwin/include
    /usr/openwin/share/include
    /opt/graphics/OpenGL/include
    /usr/include)

set(Xinerama_LIB_SEARCH_PATH
    /usr/X11R6/lib
    /usr/local/lib
    /usr/openwin/lib
    /usr/lib)

find_path(Xinerama_INCLUDE_DIR X11/extensions/Xinerama.h
          ${Xinerama_INC_SEARCH_PATH})

find_library(Xinerama_LIBRARIES NAMES Xinerama PATH ${Xinerama_LIB_SEARCH_PATH})

if(Xinerama_INCLUDE_DIR AND Xinerama_LIBRARIES)
  set(Xinerama_FOUND TRUE)
endif(Xinerama_INCLUDE_DIR AND Xinerama_LIBRARIES)

if(Xinerama_FOUND)
  include(CheckLibraryExists)

  check_library_exists(${Xinerama_LIBRARIES}
                       "XineramaQueryScreens"
                       ${Xinerama_LIBRARIES}
                       Xinerama_HAS_QUERY)

  if(NOT Xinerama_HAS_QUERY AND Xinerama_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find Xinerama")
  endif(NOT Xinerama_HAS_QUERY AND Xinerama_FIND_REQUIRED)
endif(Xinerama_FOUND)

mark_as_advanced(Xinerama_INCLUDE_DIR Xinerama_LIBRARIES)
