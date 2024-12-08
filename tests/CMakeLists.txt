include(CTest)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${conky_includes})

file(GLOB test_srcs test-*.cc)

if(NOT OS_LINUX)
  list(FILTER test_srcs EXCLUDE REGEX ".*linux.*\.cc?")
endif()

if(NOT OS_DARWIN)
  list(FILTER test_srcs EXCLUDE REGEX ".*darwin.*\.cc?")
endif()

add_library(Catch2 STATIC catch2/catch_amalgamated.cpp)

add_executable(test-conky test-common.cc ${test_srcs})
target_link_libraries(test-conky
  PRIVATE Catch2
  PUBLIC conky_core
)
catch_discover_tests(test-conky)

if(CODE_COVERAGE)
  set(COVERAGE_LCOV_EXCLUDES
    "*/include/c++/v1/*"
    "/usr/include/*"
    "/usr/lib/*"
    "/usr/local/Cellar/*"
    "/usr/local/include/*"
    "/usr/local/lib/*"
    "/usr/include/libkern/i386/*"
    "/usr/include/sys/_types/*"
    "/usr/local/opt/gettext/include/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/catch2/*"
    "${CMAKE_SOURCE_DIR}/3rdparty/*")

  setup_target_for_coverage_lcov_html(NAME
    test-conky-coverage-html
    EXECUTABLE
    test-conky
    DEPENDENCIES
    test-conky)
  setup_target_for_coverage_lcov_txt(NAME
    test-conky-coverage-txt
    EXECUTABLE
    test-conky
    DEPENDENCIES
    test-conky)
endif()
