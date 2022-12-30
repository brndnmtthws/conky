find_path(
  Wayland_CLIENT_INCLUDE_DIR
  NAMES wayland-client.h
)

find_library(
  Wayland_CLIENT_LIBRARY
  NAMES wayland-client libwayland-client
)

if(Wayland_CLIENT_INCLUDE_DIR AND Wayland_CLIENT_LIBRARY)
  add_library(wayland::client UNKNOWN IMPORTED)

  set_target_properties(
    wayland::client PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Wayland_CLIENT_INCLUDE_DIR}"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${Wayland_CLIENT_LIBRARY}"
  )
endif()

find_path(
  Wayland_SERVER_INCLUDE_DIR
  NAMES wayland-server.h
)

find_library(
  Wayland_SERVER_LIBRARY
  NAMES wayland-server libwayland-server
)

if(Wayland_SERVER_INCLUDE_DIR AND Wayland_SERVER_LIBRARY)
  add_library(wayland::server UNKNOWN IMPORTED)

  set_target_properties(
    wayland::server PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Wayland_SERVER_INCLUDE_DIR}"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${Wayland_SERVER_LIBRARY}"
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Wayland
  REQUIRED_VARS Wayland_CLIENT_LIBRARY Wayland_CLIENT_INCLUDE_DIR Wayland_SERVER_LIBRARY Wayland_SERVER_INCLUDE_DIR
)

mark_as_advanced(
  Wayland_CLIENT_INCLUDE_DIR
  Wayland_CLIENT_LIBRARY
  Wayland_SERVER_INCLUDE_DIR
  Wayland_SERVER_LIBRARY
)
