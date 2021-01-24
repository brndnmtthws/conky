# Find Clang format
#

set(ClangFormat_BIN_NAME
    clang-format
    clang-format-5.0
    clang-format-6.0
    clang-format-7
    clang-format-8)

find_program(ClangFormat_BIN NAMES ${ClangFormat_BIN_NAME})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangFormat DEFAULT_MSG ClangFormat_BIN)

mark_as_advanced(ClangFormat_BIN)

if(ClangFormat_FOUND)
  # A CMake script to find all source files and setup clang-format targets for
  # them
  include(clang-format)
else()
  message("clang-format not found. Not setting up format targets")
endif()
