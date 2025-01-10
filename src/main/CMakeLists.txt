add_executable(conky ../main.cc)
target_link_libraries(conky conky-options conky-dependencies conky-core)

install(TARGETS conky
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

print_target_properties(conky)
