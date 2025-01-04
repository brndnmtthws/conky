# cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0028 NEW) # Double colon in target name means ALIAS or IMPORTED target.
cmake_policy(SET CMP0048 NEW) # The ``project()`` command manages VERSION variables.

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
   message(FATAL_ERROR "You don't want to configure in the source directory!")
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebug RelWithDebInfo MinSizeRel."
      FORCE)
endif()

# read version parts from version.h
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/Vc/version.h _version_lines REGEX "^#define Vc_VERSION_STRING ")
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _version_matches "${_version_lines}")

project(Vc VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(disabled_targets)

include (VcMacros)
include (AddTargetProperty)
include (OptimizeForArchitecture)

vc_determine_compiler()

if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(i686|x86|AMD64|amd64)")
   set(Vc_X86 TRUE)
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(arm|aarch32|aarch64)")
   message(WARNING "No optimized implementation of the Vc types available for ${CMAKE_SYSTEM_PROCESSOR}")
   set(Vc_ARM TRUE)
else()
   message(WARNING "No optimized implementation of the Vc types available for ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(NOT Vc_COMPILER_IS_MSVC)
   AddCompilerFlag("-std=c++14" CXX_RESULT _ok CXX_FLAGS CMAKE_CXX_FLAGS)
   if(NOT _ok)
      AddCompilerFlag("-std=c++1y" CXX_RESULT _ok CXX_FLAGS CMAKE_CXX_FLAGS)
      if(NOT _ok)
         AddCompilerFlag("-std=c++11" CXX_RESULT _ok CXX_FLAGS CMAKE_CXX_FLAGS)
         if(NOT _ok)
            AddCompilerFlag("-std=c++0x" CXX_RESULT _ok CXX_FLAGS CMAKE_CXX_FLAGS)
            if(NOT _ok)
               message(FATAL_ERROR "Vc 1.x requires C++11, better even C++14. It seems this is not available. If this was incorrectly determined please notify vc-devel@compeng.uni-frankfurt.de")
            endif()
         endif()
      endif()
   endif()
# elseif(MSVC_VERSION LESS 1920)
#    message(FATAL_ERROR "Vc 1.x requires at least Visual Studio 2019.")
#    AddCompilerFlag("/std:c++14" CXX_RESULT _ok CXX_FLAGS CMAKE_CXX_FLAGS)
endif()

# if(MSVC AND (NOT DEFINED Vc_USE_MSVC_SSA_OPTIMIZER_DESPITE_BUGGY_EXP OR NOT Vc_USE_MSVC_SSA_OPTIMIZER_DESPITE_BUGGY_EXP))
#    # bug report: https://developercommunity.visualstudio.com/t/AVX-codegen-bug-on-Vc-with-MSVC-2019/1470844#T-N1521672
#    message(STATUS "WARNING! MSVC starting with 19.20 uses a new optimizer that has a bug causing Vc::exp() to return slighly wrong results.\
#  You can set Vc_USE_MSVC_SSA_OPTIMIZER_DESPITE_BUGGY_EXP=ON to still use the new optimizer on the affected MSVC versions.")
#    AddCompilerFlag("/d2SSAOptimizer-" CXX_RESULT _ok CXX_FLAGS CMAKE_CXX_FLAGS)
# endif()

if(Vc_COMPILER_IS_GCC)
   if(Vc_GCC_VERSION VERSION_GREATER "5.0.0" AND Vc_GCC_VERSION VERSION_LESS "6.0.0")
      UserWarning("GCC 5 goes into an endless loop comiling example_scaling_scalar. Therefore, this target is disabled.")
      list(APPEND disabled_targets
         example_scaling_scalar
         )
   endif()
# elseif(Vc_COMPILER_IS_MSVC)
#    # Disable warning "C++ exception specification ignored except to indicate a function is not __declspec(nothrow)"
#    # MSVC emits the warning for the _UnitTest_Compare desctructor which needs the throw declaration so that it doesn't std::terminate
#    AddCompilerFlag("/wd4290")
endif()

vc_set_preferred_compiler_flags(WARNING_FLAGS BUILDTYPE_FLAGS)

add_definitions(${Vc_DEFINITIONS})
add_compile_options(${Vc_COMPILE_FLAGS})

if(Vc_COMPILER_IS_INTEL)
   # per default icc is not IEEE compliant, but we need that for verification
   AddCompilerFlag("-fp-model source")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "" AND NOT CMAKE_CXX_FLAGS MATCHES "-O[123]")
   message(STATUS "WARNING! It seems you are compiling without optimization. Please set CMAKE_BUILD_TYPE.")
endif(CMAKE_BUILD_TYPE STREQUAL "" AND NOT CMAKE_CXX_FLAGS MATCHES "-O[123]")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}) # ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_custom_target(other VERBATIM)
add_custom_target(Scalar COMMENT "build Scalar code" VERBATIM)
add_custom_target(SSE COMMENT "build SSE code" VERBATIM)
add_custom_target(AVX COMMENT "build AVX code" VERBATIM)
add_custom_target(AVX2 COMMENT "build AVX2 code" VERBATIM)

AddCompilerFlag(-ftemplate-depth=128 CXX_FLAGS CMAKE_CXX_FLAGS)

set(libvc_compile_flags "-DVc_COMPILE_LIB")
AddCompilerFlag("-fPIC" CXX_FLAGS libvc_compile_flags)

# -fstack-protector is the default of GCC, but at least Ubuntu changes the default to -fstack-protector-strong, which is crazy
AddCompilerFlag("-fstack-protector" CXX_FLAGS libvc_compile_flags)

set(_srcs src/const.cpp)
if(Vc_X86)
   list(APPEND _srcs src/cpuid.cpp src/support_x86.cpp)
   vc_compile_for_all_implementations(_srcs src/trigonometric.cpp ONLY SSE2 SSE3 SSSE3 SSE4_1 AVX AVX+FMA AVX2+FMA+BMI2)
   if(NOT Vc_XOP_INTRINSICS_BROKEN)
     vc_compile_for_all_implementations(_srcs src/trigonometric.cpp ONLY AVX+XOP+FMA)
     if(NOT Vc_FMA4_INTRINSICS_BROKEN)
       vc_compile_for_all_implementations(_srcs src/trigonometric.cpp ONLY SSE+XOP+FMA4 AVX+XOP+FMA4)
     endif()
   endif()
   vc_compile_for_all_implementations(_srcs src/sse_sorthelper.cpp ONLY SSE2 SSE4_1 AVX AVX2+FMA+BMI2)
   vc_compile_for_all_implementations(_srcs src/avx_sorthelper.cpp ONLY AVX AVX2+FMA+BMI2)
elseif(Vc_ARM)
   list(APPEND _srcs src/support_dummy.cpp)
else()
   list(APPEND _srcs src/support_dummy.cpp)
endif()
add_library(Vc STATIC ${_srcs})
set_property(TARGET Vc APPEND PROPERTY COMPILE_OPTIONS ${libvc_compile_flags})
add_target_property(Vc LABELS "other")
if(XCODE)
   # TODO: document what this does and why it has no counterpart in the non-XCODE logic
   set_target_properties(Vc PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
   set_target_properties(Vc PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "YES")
   set_target_properties(Vc PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
   set_target_properties(Vc PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
elseif(UNIX AND Vc_COMPILER_IS_CLANG)
   # On UNIX (Linux) the standard library used by default typically is libstdc++ (GCC).
   # To get the full clang deal we rather want to build against libc++. This requires
   # additionally the libc++abi and libsupc++ libraries in all linker invokations.
   option(USE_LIBC++ "Use libc++ instead of the system default C++ standard library." OFF)
   if(USE_LIBC++)
      AddCompilerFlag(-stdlib=libc++ CXX_FLAGS CMAKE_CXX_FLAGS CXX_RESULT _use_libcxx)
      if(_use_libcxx)
         find_library(LIBC++ABI c++abi)
         mark_as_advanced(LIBC++ABI)
         if(LIBC++ABI)
            set(CMAKE_REQUIRED_LIBRARIES "${LIBC++ABI};supc++")
            CHECK_CXX_SOURCE_COMPILES("#include <stdexcept>
            #include <iostream>
            void foo() {
              std::cout << 'h' << std::flush << std::endl;
              throw std::exception();
            }
            int main() {
              try { foo(); }
              catch (int) { return 0; }
              return 1;
            }" libcxx_compiles)
            unset(CMAKE_REQUIRED_LIBRARIES)
            if(libcxx_compiles)
               link_libraries(${LIBC++ABI} supc++)
            endif()
         endif()
      endif()
   else()
      CHECK_CXX_SOURCE_COMPILES("#include <tuple>
      std::tuple<int> f() { std::tuple<int> r; return r; }
      int main() { return 0; }
      " tuple_sanity)
      if (NOT tuple_sanity)
         message(FATAL_ERROR "Clang and std::tuple brokenness detected. Please update your compiler.")
      endif()
   endif()
endif()
add_dependencies(other Vc)
target_include_directories(Vc
   PUBLIC
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
   $<INSTALL_INTERFACE:include>
   )

# option(Vc_ENABLE_INSTALL "Whether to install the library." OFF)
# if (Vc_ENABLE_INSTALL)
#    install(TARGETS Vc EXPORT VcTargets DESTINATION lib${LIB_SUFFIX})
#    install(DIRECTORY Vc/ DESTINATION include/Vc FILES_MATCHING REGEX "/*.(h|tcc|def)$")
#    install(FILES
#       Vc/Allocator
#       Vc/IO
#       Vc/Memory
#       Vc/SimdArray
#       Vc/Utils
#       Vc/Vc
#       Vc/algorithm
#       Vc/array
#       Vc/iterators
#       Vc/limits
#       Vc/simdize
#       Vc/span
#       Vc/type_traits
#       Vc/vector
#       DESTINATION include/Vc)

#    # Generate and install CMake package and modules
#    include(CMakePackageConfigHelpers)
#    set(PACKAGE_INSTALL_DESTINATION
#       lib${LIB_SUFFIX}/cmake/${PROJECT_NAME}
#       )
#    install(EXPORT ${PROJECT_NAME}Targets
#       NAMESPACE ${PROJECT_NAME}::
#       DESTINATION ${PACKAGE_INSTALL_DESTINATION}
#       EXPORT_LINK_INTERFACE_LIBRARIES
#       )
#    write_basic_package_version_file(
#       ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake
#       VERSION ${PROJECT_VERSION}
#       COMPATIBILITY AnyNewerVersion
#       )
#    configure_package_config_file(
#       ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
#       ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake
#       INSTALL_DESTINATION ${PACKAGE_INSTALL_DESTINATION}
#       PATH_VARS CMAKE_INSTALL_PREFIX
#       )
#    install(FILES
#       cmake/UserWarning.cmake
#       cmake/VcMacros.cmake
#       cmake/AddCompilerFlag.cmake
#       cmake/CheckCCompilerFlag.cmake
#       cmake/CheckCXXCompilerFlag.cmake
#       cmake/OptimizeForArchitecture.cmake
#       cmake/FindVc.cmake
#       ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake
#       ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake
#       DESTINATION ${PACKAGE_INSTALL_DESTINATION}
#       )
# endif()

# option(BUILD_TESTING "Build the testing tree." OFF)
# include (CTest)
# configure_file(${PROJECT_SOURCE_DIR}/CTestCustom.cmake ${PROJECT_BINARY_DIR}/CTestCustom.cmake COPYONLY)
# if(BUILD_TESTING)
   # add_custom_target(build_tests ALL VERBATIM)
   # add_subdirectory(tests)
# endif()

# set(BUILD_EXAMPLES FALSE CACHE BOOL "Build examples.")
# if(BUILD_EXAMPLES)
   # add_subdirectory(examples)
# endif(BUILD_EXAMPLES)

# Hide Vc_IMPL as it is only meant for users of Vc
mark_as_advanced(Vc_IMPL)

# find_program(BIN_CAT cat)
# mark_as_advanced(BIN_CAT)
# if(BIN_CAT)
#    file(REMOVE ${PROJECT_BINARY_DIR}/help.txt)
#    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/help.txt
#       COMMAND ${CMAKE_MAKE_PROGRAM} help > ${PROJECT_BINARY_DIR}/help.txt
#       VERBATIM
#       )
#    add_custom_target(cached_help
#       ${BIN_CAT} ${PROJECT_BINARY_DIR}/help.txt
#       DEPENDS ${PROJECT_BINARY_DIR}/help.txt
#       VERBATIM
#       )
# endif()
