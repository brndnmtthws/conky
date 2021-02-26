get_target_property(ClangTidy_SRCS_TMP conky SOURCES)
get_target_property(conky_SRC_DIR conky SOURCE_DIR)

if(BUILD_TESTS)
  get_target_property(ClangTidy_SRCS_TMP_CORE conky_core SOURCES)
  list(APPEND ClangTidy_SRCS_TMP ${ClangTidy_SRCS_TMP_CORE})
endif()

foreach(TMP_SRC ${ClangTidy_SRCS_TMP})
  if("${TMP_SRC}" MATCHES ".*\.cc|.*\.hh|.*\.[chi]pp|.*\.[chi]xx|.*\.ii")
    list(APPEND ClangTidy_SRCS ${conky_SRC_DIR}/${TMP_SRC})
  endif()
endforeach(TMP_SRC)

get_target_property(CLANG_INCLUDES_tmp conky INCLUDE_DIRECTORIES)
foreach(TMP_INCLUDE ${CLANG_INCLUDES_tmp})
  list(APPEND CLANG_INCLUDES -I${TMP_INCLUDE})
endforeach(TMP_INCLUDE)

add_custom_target(clang-tidy
                  COMMAND ${ClangTidy_BIN} -config='' -fix -format-style=file
                          ${ClangTidy_SRCS}
                          -- -std=c++17 -I${CMAKE_BINARY_DIR} ${CLANG_INCLUDES})
add_custom_target(check-clang-tidy
                  COMMAND ${ClangTidy_BIN} -config='' -format-style=file
                          ${ClangTidy_SRCS}
                          -- -std=c++17 -I${CMAKE_BINARY_DIR} ${CLANG_INCLUDES})
