include_guard()

include(CheckCXXCompilerFlag)
include(FindPackageHandleStandardArgs)

if("${DPCPP_SYCL_TARGET}" STREQUAL "amdgcn-amd-amdhsa" AND
   "${DPCPP_SYCL_ARCH}" STREQUAL "")
   message(FATAL_ERROR "Architecture required for AMD DPCPP builds,"
                       " please specify in DPCPP_SYCL_ARCH")
endif()

set(DPCPP_USER_FLAGS "" CACHE STRING 
    "Additional user-specified compiler flags for DPC++")

get_filename_component(DPCPP_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
find_library(DPCPP_LIB_DIR NAMES sycl sycl6 PATHS "${DPCPP_BIN_DIR}/../lib")

add_library(DPCPP::DPCPP INTERFACE IMPORTED)

set(DPCPP_FLAGS "-fsycl;-fsycl-targets=${DPCPP_SYCL_TARGET};-fsycl-unnamed-lambda;${DPCPP_USER_FLAGS};-ftemplate-backtrace-limit=0")
if(NOT "${DPCPP_SYCL_ARCH}" STREQUAL "")
  if("${DPCPP_SYCL_TARGET}" STREQUAL "amdgcn-amd-amdhsa")
    list(APPEND DPCPP_FLAGS "-Xsycl-target-backend")
    list(APPEND DPCPP_FLAGS "--offload-arch=${DPCPP_SYCL_ARCH}")
  elseif("${DPCPP_SYCL_TARGET}" STREQUAL "nvptx64-nvidia-cuda")
    list(APPEND DPCPP_FLAGS "-Xsycl-target-backend")
    list(APPEND DPCPP_FLAGS "--cuda-gpu-arch=${DPCPP_SYCL_ARCH}")
  endif()
endif()

if(UNIX)
  set_target_properties(DPCPP::DPCPP PROPERTIES
    INTERFACE_COMPILE_OPTIONS "${DPCPP_FLAGS}"
    INTERFACE_LINK_OPTIONS "${DPCPP_FLAGS}"
    INTERFACE_LINK_LIBRARIES ${DPCPP_LIB_DIR}
    INTERFACE_INCLUDE_DIRECTORIES "${DPCPP_BIN_DIR}/../include/sycl;${DPCPP_BIN_DIR}/../include")
    message(STATUS ">>>>>>>>> DPCPP INCLUDE DIR: ${DPCPP_BIN_DIR}/../include/sycl")
else()
  set_target_properties(DPCPP::DPCPP PROPERTIES
    INTERFACE_COMPILE_OPTIONS "${DPCPP_FLAGS}"
    INTERFACE_LINK_LIBRARIES ${DPCPP_LIB_DIR}
    INTERFACE_INCLUDE_DIRECTORIES "${DPCPP_BIN_DIR}/../include/sycl")
endif()

function(add_sycl_to_target)
  set(options)
  set(one_value_args TARGET)
  set(multi_value_args SOURCES)
  cmake_parse_arguments(SB_ADD_SYCL
    "${options}"
    "${one_value_args}"
    "${multi_value_args}"
    ${ARGN}
  )
  target_compile_options(${SB_ADD_SYCL_TARGET} PUBLIC ${DPCPP_FLAGS})
  target_link_libraries(${SB_ADD_SYCL_TARGET} DPCPP::DPCPP)
  target_compile_features(${SB_ADD_SYCL_TARGET} PRIVATE cxx_std_17)
  get_target_property(target_type ${SB_ADD_SYCL_TARGET} TYPE)
  if (NOT target_type STREQUAL "OBJECT_LIBRARY")
    target_link_options(${SB_ADD_SYCL_TARGET} PUBLIC ${DPCPP_FLAGS})
  endif()
endfunction()
