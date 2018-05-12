# Find Clang tidy
#

if(NOT CLANG_TIDY_BIN_NAME)
	set(CLANG_TIDY_BIN_NAME clang-tidy)
endif()

# if custom path check there first
if(CLANG_TIDY_ROOT_DIR)
	find_program(CLANG_TIDY_BIN
		NAMES
		${CLANG_TIDY_BIN_NAME}
		PATHS
		"${CLANG_TIDY_ROOT_DIR}"
	NO_DEFAULT_PATH)
endif()

find_program(CLANG_TIDY_BIN NAMES ${CLANG_TIDY_BIN_NAME})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	CLANG_TIDY
	DEFAULT_MSG
CLANG_TIDY_BIN)

mark_as_advanced(
	CLANG_TIDY_BIN
)

if(CLANG_TIDY_FOUND)
	# A CMake script to find all source files and setup clang-tidy targets for them
	include(clang-tidy)
else()
	message("clang-tidy not found. Not setting up tidy targets")
endif()
