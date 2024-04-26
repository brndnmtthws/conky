project(EigenLapack CXX)

if(EIGEN_BUILD_LAPACK AND EIGEN_BUILD_BLAS)

include(CheckLanguage)
check_language(Fortran)
if(CMAKE_Fortran_COMPILER)
  enable_language(Fortran)
  if("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "GNU")
    if ("${CMAKE_Fortran_COMPILER_VERSION}" VERSION_GREATER_EQUAL 10.0)
      # We use an old version of LAPACK with argument type mismatches.
      # Allow them to compile anyway with newer GNU versions.
      set(CMAKE_Fortran_FLAGS  "${CMAKE_Fortran_FLAGS} -fallow-argument-mismatch")
    endif()
  endif()
  set(EIGEN_Fortran_COMPILER_WORKS ON)
else()
  set(EIGEN_Fortran_COMPILER_WORKS OFF)
endif()

add_custom_target(lapack)
include_directories(../blas)

set(EigenLapack_SRCS
  dsecnd_INT_CPU_TIME.cpp second_INT_CPU_TIME.cpp single.cpp double.cpp complex_single.cpp complex_double.cpp ../blas/xerbla.cpp
)

if(EIGEN_Fortran_COMPILER_WORKS)

set(EigenLapack_SRCS  ${EigenLapack_SRCS}
  slarft.f  dlarft.f  clarft.f  zlarft.f
  slarfb.f  dlarfb.f  clarfb.f  zlarfb.f
  slarfg.f  dlarfg.f  clarfg.f  zlarfg.f
  slarf.f   dlarf.f   clarf.f   zlarf.f
  sladiv.f  dladiv.f  cladiv.f  zladiv.f
  ilaslr.f  iladlr.f  ilaclr.f  ilazlr.f
  ilaslc.f  iladlc.f  ilaclc.f  ilazlc.f
  dlapy2.f  dlapy3.f  slapy2.f  slapy3.f
  clacgv.f  zlacgv.f
  slamch.f  dlamch.f
)

option(EIGEN_ENABLE_LAPACK_TESTS OFF "Enable the Lapack unit tests")

if(EIGEN_ENABLE_LAPACK_TESTS)

  get_filename_component(eigen_full_path_to_reference_lapack "./reference/" ABSOLUTE)
  if(NOT EXISTS ${eigen_full_path_to_reference_lapack})
    # Download lapack and install sources and testing at the right place
    message(STATUS "Download lapack_addons_3.4.1.tgz...")
    
    file(DOWNLOAD "http://downloads.tuxfamily.org/eigen/lapack_addons_3.4.1.tgz"
                  "${CMAKE_CURRENT_SOURCE_DIR}/lapack_addons_3.4.1.tgz"
                  INACTIVITY_TIMEOUT 15
                  TIMEOUT 240
                  STATUS download_status
                  EXPECTED_MD5 ab5742640617e3221a873aba44bbdc93
                  SHOW_PROGRESS)
                  
    message(STATUS ${download_status})
    list(GET download_status 0 download_status_num)
    set(download_status_num 0)
    if(download_status_num EQUAL 0)
      message(STATUS "Setup lapack reference and lapack unit tests")
      execute_process(COMMAND tar xzf  "lapack_addons_3.4.1.tgz" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    else()
      message(STATUS "Download of lapack_addons_3.4.1.tgz failed, LAPACK unit tests won't be enabled")
      set(EIGEN_ENABLE_LAPACK_TESTS false)
    endif()
                  
  endif()
  
  get_filename_component(eigen_full_path_to_reference_lapack "./reference/" ABSOLUTE)
  if(EXISTS ${eigen_full_path_to_reference_lapack})
    set(EigenLapack_funcfilenames
        ssyev.f   dsyev.f   csyev.f   zsyev.f
        spotrf.f  dpotrf.f  cpotrf.f  zpotrf.f
        spotrs.f  dpotrs.f  cpotrs.f  zpotrs.f
        sgetrf.f  dgetrf.f  cgetrf.f  zgetrf.f
        sgetrs.f  dgetrs.f  cgetrs.f  zgetrs.f)
    
    file(GLOB ReferenceLapack_SRCS0 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "reference/*.f")
    foreach(filename1 IN LISTS ReferenceLapack_SRCS0)
      string(REPLACE "reference/" "" filename ${filename1})
      list(FIND EigenLapack_SRCS ${filename} id1)
      list(FIND EigenLapack_funcfilenames ${filename} id2)
      if((id1 EQUAL -1) AND (id2 EQUAL -1))
        set(ReferenceLapack_SRCS ${ReferenceLapack_SRCS} reference/${filename})
      endif()
    endforeach()
  endif()
  
  
endif()

endif()

set(EIGEN_LAPACK_TARGETS "")

add_library(eigen_lapack_static ${EigenLapack_SRCS} ${ReferenceLapack_SRCS})
list(APPEND EIGEN_LAPACK_TARGETS eigen_lapack_static)

if (EIGEN_BUILD_SHARED_LIBS)
  add_library(eigen_lapack SHARED ${EigenLapack_SRCS})
  list(APPEND EIGEN_LAPACK_TARGETS eigen_lapack)
  target_link_libraries(eigen_lapack  eigen_blas)
endif()

foreach(target IN LISTS EIGEN_LAPACK_TARGETS)
  if(EIGEN_STANDARD_LIBRARIES_TO_LINK_TO)
    target_link_libraries(${target} ${EIGEN_STANDARD_LIBRARIES_TO_LINK_TO})
  endif()
  add_dependencies(lapack ${target})
  install(TARGETS ${target}
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib)
endforeach()


get_filename_component(eigen_full_path_to_testing_lapack "./testing/" ABSOLUTE)
if(EXISTS ${eigen_full_path_to_testing_lapack})
  
  # The following comes from lapack/TESTING/CMakeLists.txt
  # Get Python
  find_package(PythonInterp)
  message(STATUS "Looking for Python found - ${PYTHONINTERP_FOUND}")
  if (PYTHONINTERP_FOUND)
    message(STATUS "Using Python version ${PYTHON_VERSION_STRING}")
  endif()

  set(LAPACK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(LAPACK_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
  set(BUILD_SINGLE      true)
  set(BUILD_DOUBLE      true)
  set(BUILD_COMPLEX     true)
  set(BUILD_COMPLEX16E  true)
  
  if(MSVC_VERSION)
#  string(REPLACE "/STACK:10000000" "/STACK:900000000000000000"
#    CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
  string(REGEX REPLACE "(.*)/STACK:(.*) (.*)" "\\1/STACK:900000000000000000 \\3"
    CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
  endif()
  file(MAKE_DIRECTORY "${LAPACK_BINARY_DIR}/TESTING")
  add_subdirectory(testing/MATGEN)
  add_subdirectory(testing/LIN)
  add_subdirectory(testing/EIG)
  macro(add_lapack_test output input target)
    set(TEST_INPUT "${LAPACK_SOURCE_DIR}/testing/${input}")
    set(TEST_OUTPUT "${LAPACK_BINARY_DIR}/TESTING/${output}")
    string(REPLACE "." "_" input_name ${input})
    set(testName "${target}_${input_name}")
    if(EXISTS "${TEST_INPUT}")
      add_dependencies(buildtests ${target})
      add_test(NAME LAPACK-${testName}
        COMMAND "${CMAKE_COMMAND}"
        -DTEST=$<TARGET_FILE:${target}>
        -DINPUT=${TEST_INPUT} 
        -DOUTPUT=${TEST_OUTPUT} 
        -DINTDIR=${CMAKE_CFG_INTDIR}
        -P "${LAPACK_SOURCE_DIR}/testing/runtest.cmake")
    endif()
  endmacro()

  if (BUILD_SINGLE)
  add_lapack_test(stest.out stest.in xlintsts)
  #
  # ======== SINGLE RFP LIN TESTS ========================
  add_lapack_test(stest_rfp.out stest_rfp.in xlintstrfs)
  #
  #
  # ======== SINGLE EIG TESTS ===========================
  #

  add_lapack_test(snep.out nep.in xeigtsts)


  add_lapack_test(ssep.out sep.in xeigtsts)


  add_lapack_test(ssvd.out svd.in xeigtsts)


  add_lapack_test(sec.out sec.in xeigtsts)


  add_lapack_test(sed.out sed.in xeigtsts)


  add_lapack_test(sgg.out sgg.in xeigtsts)


  add_lapack_test(sgd.out sgd.in xeigtsts)


  add_lapack_test(ssb.out ssb.in xeigtsts)


  add_lapack_test(ssg.out ssg.in xeigtsts)


  add_lapack_test(sbal.out sbal.in xeigtsts)


  add_lapack_test(sbak.out sbak.in xeigtsts)


  add_lapack_test(sgbal.out sgbal.in xeigtsts)


  add_lapack_test(sgbak.out sgbak.in xeigtsts)


  add_lapack_test(sbb.out sbb.in xeigtsts)


  add_lapack_test(sglm.out glm.in xeigtsts)


  add_lapack_test(sgqr.out gqr.in xeigtsts)


  add_lapack_test(sgsv.out gsv.in xeigtsts)


  add_lapack_test(scsd.out csd.in xeigtsts)


  add_lapack_test(slse.out lse.in xeigtsts)
  endif()

  if (BUILD_DOUBLE)
  #
  # ======== DOUBLE LIN TESTS ===========================
  add_lapack_test(dtest.out dtest.in xlintstd)
  #
  # ======== DOUBLE RFP LIN TESTS ========================
  add_lapack_test(dtest_rfp.out dtest_rfp.in xlintstrfd)
  #
  # ======== DOUBLE EIG TESTS ===========================

  add_lapack_test(dnep.out nep.in xeigtstd)


  add_lapack_test(dsep.out sep.in xeigtstd)


  add_lapack_test(dsvd.out svd.in xeigtstd)


  add_lapack_test(dec.out dec.in xeigtstd)


  add_lapack_test(ded.out ded.in xeigtstd)


  add_lapack_test(dgg.out dgg.in xeigtstd)


  add_lapack_test(dgd.out dgd.in xeigtstd)


  add_lapack_test(dsb.out dsb.in xeigtstd)


  add_lapack_test(dsg.out dsg.in xeigtstd)


  add_lapack_test(dbal.out dbal.in xeigtstd)


  add_lapack_test(dbak.out dbak.in xeigtstd)


  add_lapack_test(dgbal.out dgbal.in xeigtstd)


  add_lapack_test(dgbak.out dgbak.in xeigtstd)


  add_lapack_test(dbb.out dbb.in xeigtstd)


  add_lapack_test(dglm.out glm.in xeigtstd)


  add_lapack_test(dgqr.out gqr.in xeigtstd)


  add_lapack_test(dgsv.out gsv.in xeigtstd)


  add_lapack_test(dcsd.out csd.in xeigtstd)


  add_lapack_test(dlse.out lse.in xeigtstd)
  endif()

  if (BUILD_COMPLEX)
  add_lapack_test(ctest.out ctest.in xlintstc)
  #
  # ======== COMPLEX RFP LIN TESTS ========================
  add_lapack_test(ctest_rfp.out ctest_rfp.in xlintstrfc)
  #
  # ======== COMPLEX EIG TESTS ===========================

  add_lapack_test(cnep.out nep.in xeigtstc)


  add_lapack_test(csep.out sep.in xeigtstc)


  add_lapack_test(csvd.out svd.in xeigtstc)


  add_lapack_test(cec.out cec.in xeigtstc)


  add_lapack_test(ced.out ced.in xeigtstc)


  add_lapack_test(cgg.out cgg.in xeigtstc)


  add_lapack_test(cgd.out cgd.in xeigtstc)


  add_lapack_test(csb.out csb.in xeigtstc)


  add_lapack_test(csg.out csg.in xeigtstc)


  add_lapack_test(cbal.out cbal.in xeigtstc)


  add_lapack_test(cbak.out cbak.in xeigtstc)


  add_lapack_test(cgbal.out cgbal.in xeigtstc)


  add_lapack_test(cgbak.out cgbak.in xeigtstc)


  add_lapack_test(cbb.out cbb.in xeigtstc)


  add_lapack_test(cglm.out glm.in xeigtstc)


  add_lapack_test(cgqr.out gqr.in xeigtstc)


  add_lapack_test(cgsv.out gsv.in xeigtstc)


  add_lapack_test(ccsd.out csd.in xeigtstc)


  add_lapack_test(clse.out lse.in xeigtstc)
  endif()

  if (BUILD_COMPLEX16)
  #
  # ======== COMPLEX16 LIN TESTS ========================
  add_lapack_test(ztest.out ztest.in xlintstz)
  #
  # ======== COMPLEX16 RFP LIN TESTS ========================
  add_lapack_test(ztest_rfp.out ztest_rfp.in xlintstrfz)
  #
  # ======== COMPLEX16 EIG TESTS ===========================

  add_lapack_test(znep.out nep.in xeigtstz)


  add_lapack_test(zsep.out sep.in xeigtstz)


  add_lapack_test(zsvd.out svd.in xeigtstz)


  add_lapack_test(zec.out zec.in xeigtstz)


  add_lapack_test(zed.out zed.in xeigtstz)


  add_lapack_test(zgg.out zgg.in xeigtstz)


  add_lapack_test(zgd.out zgd.in xeigtstz)


  add_lapack_test(zsb.out zsb.in xeigtstz)


  add_lapack_test(zsg.out zsg.in xeigtstz)


  add_lapack_test(zbal.out zbal.in xeigtstz)


  add_lapack_test(zbak.out zbak.in xeigtstz)


  add_lapack_test(zgbal.out zgbal.in xeigtstz)


  add_lapack_test(zgbak.out zgbak.in xeigtstz)


  add_lapack_test(zbb.out zbb.in xeigtstz)


  add_lapack_test(zglm.out glm.in xeigtstz)


  add_lapack_test(zgqr.out gqr.in xeigtstz)


  add_lapack_test(zgsv.out gsv.in xeigtstz)


  add_lapack_test(zcsd.out csd.in xeigtstz)


  add_lapack_test(zlse.out lse.in xeigtstz)
  endif()


  if (BUILD_SIMPLE)
      if (BUILD_DOUBLE)
  #
  # ======== SINGLE-DOUBLE PROTO LIN TESTS ==============
          add_lapack_test(dstest.out dstest.in xlintstds)
      endif()
  endif()


  if (BUILD_COMPLEX)
      if (BUILD_COMPLEX16)
  #
  # ======== COMPLEX-COMPLEX16 LIN TESTS ========================
          add_lapack_test(zctest.out zctest.in xlintstzc)
      endif()
  endif()

  # ==============================================================================

  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${LAPACK_SOURCE_DIR}/testing/lapack_testing.py ${LAPACK_BINARY_DIR})
  add_test(
    NAME LAPACK_Test_Summary
    WORKING_DIRECTORY ${LAPACK_BINARY_DIR}
    COMMAND ${PYTHON_EXECUTABLE} "lapack_testing.py"
  )

endif()

elseif(EIGEN_BUILD_LAPACK AND NOT EIGEN_BUILD_BLAS)
 message(FATAL_ERROR "EIGEN_BUILD_LAPACK requires EIGEN_BUILD_BLAS")
endif() #EIGEN_BUILD_LAPACK
