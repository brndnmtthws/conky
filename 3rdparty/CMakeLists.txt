## toluapp
add_subdirectory(toluapp)
set(toluapp_includes ${CMAKE_CURRENT_SOURCE_DIR}/toluapp/include)
set(conky_includes ${conky_includes} ${toluapp_includes})
set(conky_libs ${conky_libs} toluapp_lib_static)

# Update values in global scope
set(toluapp_includes ${toluapp_includes} PARENT_SCOPE)
set(conky_includes ${conky_includes} PARENT_SCOPE)
set(conky_libs ${conky_libs} PARENT_SCOPE)
