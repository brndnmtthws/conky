if (Accelerate_INCLUDES AND Accelerate_LIBRARIES)
  set(Accelerate_FIND_QUIETLY TRUE)
endif ()

find_path(Accelerate_INCLUDES
  NAMES
  Accelerate.h
  PATHS $ENV{ACCELERATEDIR}
)

find_library(Accelerate_LIBRARIES Accelerate PATHS $ENV{ACCELERATEDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Accelerate DEFAULT_MSG
                                  Accelerate_INCLUDES Accelerate_LIBRARIES)

if (Accelerate_FOUND)
  get_filename_component(Accelerate_PARENTDIR ${Accelerate_INCLUDES} DIRECTORY)

  file(GLOB_RECURSE SparseHeader ${Accelerate_PARENTDIR}/Sparse.h)

  if ("${SparseHeader}" STREQUAL "")
    message(STATUS "Accelerate sparse matrix support was not found. Accelerate has been disabled.")
    set(Accelerate_FOUND FALSE)
  endif ()
endif ()

mark_as_advanced(Accelerate_INCLUDES Accelerate_LIBRARIES)
