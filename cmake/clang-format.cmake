set(ClangFormat_CXX_FILE_EXTENSIONS
    ${ClangFormat_CXX_FILE_EXTENSIONS}
    *.cpp
    *.h
    *.cxx
    *.hxx
    *.hpp
    *.cc
    *.hh
    *.ipp)

foreach(PATTERN ${ClangFormat_CXX_FILE_EXTENSIONS})
  list(APPEND ClangFormat_CXX_PATTERN ${CMAKE_SOURCE_DIR}/src/${PATTERN})
  list(APPEND ClangFormat_CXX_PATTERN ${CMAKE_SOURCE_DIR}/tests/${PATTERN})
  list(APPEND ClangFormat_CXX_PATTERN ${CMAKE_SOURCE_DIR}/lua/${PATTERN})
endforeach()

file(GLOB_RECURSE ClangFormat_SRCS ${ClangFormat_CXX_PATTERN})

add_custom_target(clang-format
                  COMMAND ${ClangFormat_BIN} -style=file -i
                          ${ClangFormat_SRCS})
add_custom_target(check-clang-format
                  COMMAND ${CMAKE_SOURCE_DIR}/bin/run-clang-format.py
                          --color always
                          --clang-format-executable ${ClangFormat_BIN}
                                                    ${ClangFormat_SRCS})
