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
target_include_directories(test-conky PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR})
target_link_libraries(test-conky
  PRIVATE Catch2
  PUBLIC conky_core
)
catch_discover_tests(test-conky)

if(BUILD_X11)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_dependencies(test-conky conky)
    add_test(NAME x11_startup_fallback
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test-x11-startup.py $<TARGET_FILE:conky>)
    set_tests_properties(x11_startup_fallback PROPERTIES
      TIMEOUT 45 LABELS "integration;x11")

    if(OWN_WINDOW)
      add_executable(x11-test-probe x11-test-probe.cc)
      target_link_libraries(x11-test-probe PRIVATE ${X11_LIBRARIES})
      add_dependencies(test-conky x11-test-probe)
      add_test(NAME x11_reload
        COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/test-x11-reload.py
          $<TARGET_FILE:conky> $<TARGET_FILE:x11-test-probe>)
      set_tests_properties(x11_reload PROPERTIES
        TIMEOUT 90 SKIP_RETURN_CODE 77 LABELS "integration;x11"
        RESOURCE_LOCK x11_test_display)
      if(BUILD_XFT)
        add_test(NAME x11_reload_xft
          COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/test-x11-reload.py
            $<TARGET_FILE:conky> $<TARGET_FILE:x11-test-probe>)
        set_tests_properties(x11_reload_xft PROPERTIES
          TIMEOUT 90 SKIP_RETURN_CODE 77 LABELS "integration;x11"
          RESOURCE_LOCK x11_test_display ENVIRONMENT "CONKY_TEST_X11_XFT=1")
      endif()
    endif()
  endif()
endif()

if(RUN_TESTS)
  add_custom_command(TARGET test-conky
    POST_BUILD
    COMMAND ctest -C $<CONFIGURATION> --output-on-failure)
endif()

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
