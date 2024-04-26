
if (ADOLC_INCLUDES AND ADOLC_LIBRARIES)
  set(ADOLC_FIND_QUIETLY TRUE)
endif ()

find_path(ADOLC_INCLUDES
  NAMES adolc/adtl.h
  PATHS $ENV{ADOLCDIR} $ENV{ADOLCDIR}/include ${INCLUDE_INSTALL_DIR}
)

find_library(ADOLC_LIBRARIES 
  adolc 
  PATHS $ENV{ADOLCDIR} ${LIB_INSTALL_DIR} 
  PATH_SUFFIXES lib lib64)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Adolc DEFAULT_MSG
                                  ADOLC_INCLUDES ADOLC_LIBRARIES)

mark_as_advanced(ADOLC_INCLUDES ADOLC_LIBRARIES)
