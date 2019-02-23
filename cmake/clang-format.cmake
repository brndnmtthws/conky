set(CLANG_FORMAT_CXX_FILE_EXTENSIONS
    ${CLANG_FORMAT_CXX_FILE_EXTENSIONS}
    *.cpp
    *.h
    *.cxx
    *.hxx
    *.hpp
    *.cc
    *.hh
    *.ipp)

foreach(PATTERN ${CLANG_FORMAT_CXX_FILE_EXTENSIONS})
  list(APPEND CLANG_FORMAT_CXX_PATTERN ${CMAKE_SOURCE_DIR}/src/${PATTERN})
  list(APPEND CLANG_FORMAT_CXX_PATTERN ${CMAKE_SOURCE_DIR}/tests/${PATTERN})
  list(APPEND CLANG_FORMAT_CXX_PATTERN ${CMAKE_SOURCE_DIR}/lua/${PATTERN})
endforeach()

file(GLOB_RECURSE CLANG_FORMAT_SRCS ${CLANG_FORMAT_CXX_PATTERN})

add_custom_target(clang-format
                  COMMAND ${CLANG_FORMAT_BIN} -style=file -i
                          ${CLANG_FORMAT_SRCS})
add_custom_target(check-clang-format
                  COMMAND ${CMAKE_SOURCE_DIR}/bin/run-clang-format.py
                          --color always
                          --clang-format-executable ${CLANG_FORMAT_BIN}
                                                    ${CLANG_FORMAT_SRCS})
