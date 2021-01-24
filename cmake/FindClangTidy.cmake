# Find Clang tidy
#

set(ClangTidy_BIN_NAME
    clang-tidy
    clang-tidy-5.0
    clang-tidy-6.0
    clang-tidy-7
    clang-tidy-8)

find_program(ClangTidy_BIN NAMES ${ClangTidy_BIN_NAME})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangTidy DEFAULT_MSG ClangTidy_BIN)

mark_as_advanced(ClangTidy_BIN)

if(ClangTidy_FOUND)
  # A CMake script to find all source files and setup clang-tidy targets for
  # them
  include(clang-tidy)
else()
  message("clang-tidy not found. Not setting up tidy targets")
endif()
