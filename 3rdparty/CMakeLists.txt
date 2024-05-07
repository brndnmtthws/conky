# Version 1.4.4
add_subdirectory(Vc)
set(conky_libs ${conky_libs} Vc)
set(conky_includes ${conky_includes} "${CMAKE_CURRENT_SOURCE_DIR}/Vc")

set(conky_libs ${conky_libs} PARENT_SCOPE)
set(conky_includes ${conky_includes} PARENT_SCOPE)
