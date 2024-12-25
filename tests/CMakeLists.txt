include(CTest)

file(GLOB test_sources test-*.cc)
file(GLOB mock_sources mock/*.cc)

macro(EXCLUDING_ANY excluded)
  set(__condition "${ARGN}")
  string(REGEX MATCH "^IF" __match "${__condition}")
  if(__match STREQUAL "")
    message(FATAL_ERROR "EXCLUDING_ANY call missing IF keyword")
  endif()
  unset(__match)
  string(REGEX REPLACE "^IF" "" __condition "${__condition}")
  if(${__condition})
    list(FILTER test_sources EXCLUDE REGEX ".*${excluded}.*\.(cc|hh)")
    list(FILTER mock_sources EXCLUDE REGEX ".*${excluded}.*\.(cc|hh)")
  endif()
  unset(__condition)
endmacro()

excluding_any("linux"   IF NOT OS_LINUX)
excluding_any("darwin"  IF NOT OS_DARWIN)
excluding_any("x11"     IF (NOT BUILD_X11) OR OS_DARWIN)
excluding_any("wayland" IF NOT BUILD_WAYLAND)

# Mocking works because it's linked before conky_core, so the linker uses mock
# implementations instead of those that are linked later.
add_library(conky-mock OBJECT ${mock_sources})
target_include_directories(conky-mock
  PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}
  ${conky_includes}
)
add_library(Catch2 STATIC catch2/catch_amalgamated.cpp)

add_executable(test-conky test-common.cc events.cc ${test_sources})
target_include_directories(test-conky
  PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}
  ${conky_includes}
)
target_link_libraries(test-conky Catch2 conky-mock conky_core)
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
