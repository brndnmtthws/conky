# Vc version 1.4.4
add_subdirectory(Vc)

add_subdirectory(toluapp)

if(BUILD_WLAN AND OS_LINUX)
  set(linux_net_libs_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/linux")

  # Sourced from linux kernel
  # SHA: 18daea77cca626f590fb140fc11e3a43c5d41354
  # https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/
  # Only added structures and data that's used by conky to avoid cloning entire kernel

  set(conky_includes ${conky_includes} ${linux_net_libs_INCLUDE_PATH})
endif(BUILD_WLAN AND OS_LINUX)

# Reexport includes and libs
set(conky_includes ${conky_includes} PARENT_SCOPE)
set(conky_libs ${conky_libs} PARENT_SCOPE)
