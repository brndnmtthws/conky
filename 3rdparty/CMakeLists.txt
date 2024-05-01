# Vc version 1.4.4
add_subdirectory(Vc)

add_subdirectory(toluapp)

if(BUILD_WLAN AND OS_LINUX)
  set(linux_net_libs_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/linux")

  # Sourced from linux kernel
  # SHA: 97bf6af1f928216fd6c5a66e8a57bfa95a659672
  # https://git.kernel.org/pub/scm/linux/kernel/git/linville/wireless.git/tree/

  set(conky_includes ${conky_includes} ${linux_net_libs_INCLUDE_PATH})
endif(BUILD_WLAN AND OS_LINUX)

# Reexport includes and libs
set(conky_includes ${conky_includes} PARENT_SCOPE)
set(conky_libs ${conky_libs} PARENT_SCOPE)
