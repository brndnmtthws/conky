include(CTest)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${conky_includes})

set(test_srcs "")

if(OS_LINUX)
  set(test_srcs ${test_srcs} test-linux.cc)
endif()

if(OS_DARWIN)
  set(test_srcs ${test_srcs} test-darwin.cc)
endif()

add_executable(test-conky test-common.cc ${test_srcs})
target_link_libraries(test-conky conky_core)
catch_discover_tests(test-conky)

if(USING_CLANG_7)
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
