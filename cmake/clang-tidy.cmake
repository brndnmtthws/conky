# Run clang-tidy

set(DO_CLANG_TIDY
  "${CLANG_TIDY_BIN}"
  -format-style=file
  -checks=*,-clang-analyzer-alpha.*
)

if(CLANG_TIDY_BIN)
  set_target_properties(
    conky PROPERTIES
    CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
  )
endif()
