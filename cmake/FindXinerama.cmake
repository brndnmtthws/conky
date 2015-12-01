# - Find the Xinerama include file and library
#

SET(Xinerama_INC_SEARCH_PATH
    /usr/X11R6/include
    /usr/local/include
    /usr/include/X11
    /usr/openwin/include
    /usr/openwin/share/include
    /opt/graphics/OpenGL/include
    /usr/include)

SET(Xinerama_LIB_SEARCH_PATH
    /usr/X11R6/lib
    /usr/local/lib
    /usr/openwin/lib
    /usr/lib)


FIND_PATH(Xinerama_INCLUDE_DIR X11/extensions/Xinerama.h
          ${Xinerama_INC_SEARCH_PATH})

FIND_LIBRARY(Xinerama_LIBRARIES NAMES Xinerama PATH ${Xinerama_LIB_SEARCH_PATH})

IF (Xinerama_INCLUDE_DIR AND Xinerama_LIBRARIES)
    SET(Xinerama_FOUND TRUE)
ENDIF (Xinerama_INCLUDE_DIR AND Xinerama_LIBRARIES)

IF (Xinerama_FOUND)
    INCLUDE(CheckLibraryExists)

    CHECK_LIBRARY_EXISTS(${Xinerama_LIBRARIES}
                         "XineramaQueryScreens"
                         ${Xinerama_LIBRARIES}
                         Xinerama_HAS_QUERY)

    IF (NOT Xinerama_HAS_QUERY AND Xinerama_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could NOT find Xinerama")
    ENDIF (NOT Xinerama_HAS_QUERY AND Xinerama_FIND_REQUIRED)
ENDIF (Xinerama_FOUND)

MARK_AS_ADVANCED(
    Xinerama_INCLUDE_DIR
    Xinerama_LIBRARIES
)
