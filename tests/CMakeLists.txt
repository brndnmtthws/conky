include(CTest)

file(GLOB test_sources test-*.cc)

list(APPEND test_sources ${CMAKE_BINARY_DIR}/config.h)

if(NOT OS_LINUX)
  list(FILTER test_sources EXCLUDE REGEX ".*linux.*\.cc?")
endif()

if(NOT OS_DARWIN)
  list(FILTER test_sources EXCLUDE REGEX ".*darwin.*\.cc?")
endif()

if(NOT BUILD_X11)
  list(FILTER test_sources EXCLUDE REGEX ".*x11.*\.cc?")
endif()

if(NOT BUILD_WAYLAND)
  list(FILTER test_sources EXCLUDE REGEX ".*wayland.*\.cc?")
endif()

# Mocking works because it's linked before conky_core, so the linker uses mock
# implementations instead of those that are linked later.
file(GLOB mock_sources mock/*.cc)

add_library(Catch2 STATIC catch2/catch_amalgamated.cpp)

add_executable(test-conky test-common.cc ${test_sources})
target_include_directories(test-conky
  PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}
  ${conky_includes}
)
target_link_libraries(test-conky
  PRIVATE Catch2 ${mock_sources}
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
