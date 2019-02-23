# Find Clang tidy
#

set(CLANG_TIDY_BIN_NAME
    clang-tidy
    clang-tidy-5.0
    clang-tidy-6.0
    clang-tidy-7
    clang-tidy-8)

find_program(CLANG_TIDY_BIN NAMES ${CLANG_TIDY_BIN_NAME})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CLANG_TIDY DEFAULT_MSG CLANG_TIDY_BIN)

mark_as_advanced(CLANG_TIDY_BIN)

if(CLANG_TIDY_FOUND)
  # A CMake script to find all source files and setup clang-tidy targets for
  # them
  include(clang-tidy)
else()
  message("clang-tidy not found. Not setting up tidy targets")
endif()
