# Find Clang format
#

set(CLANG_FORMAT_BIN_NAME
    clang-format
    clang-format-5.0
    clang-format-6.0
    clang-format-7
    clang-format-8)

find_program(CLANG_FORMAT_BIN NAMES ${CLANG_FORMAT_BIN_NAME})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CLANG_FORMAT DEFAULT_MSG CLANG_FORMAT_BIN)

mark_as_advanced(CLANG_FORMAT_BIN)

if(CLANG_FORMAT_FOUND)
  # A CMake script to find all source files and setup clang-format targets for
  # them
  include(clang-format)
else()
  message("clang-format not found. Not setting up format targets")
endif()
