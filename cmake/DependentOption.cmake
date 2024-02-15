# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
DependentOption
--------------------
Roughly based on CMakeDependentOption:
https://github.com/Kitware/CMake/blob/master/Modules/CMakeDependentOption.cmake

Modified to so it produces warnings instead of hiding an option completely and
sets a default value.
Difference is that `depends` argument is ALWAYS a semicolon separated list of
<expr> tokens.

Argument meaning and order are the same, and there's an additional (warn)
argument which is the message printed if the end-user enabled a feature which
isn't "possible".

Actual checks are deferred until RUN_DEPENDENCY_CHECKS() is called in order to
allow out of order declaration of dependencies and dependecy graph cycles.
As the checks can affect each other they're run in a loop until the graph settles.
#]=======================================================================]

set(__DEPENDENT_OPTIONS_CHANGE_HAPPENED true)
set(__DEPENDENT_OPTIONS_LATER_INVOKED_CODE "")

macro(DEPENDENT_OPTION option doc default depends else warn)
  option(${option} "${doc}" "${default}")

  string(APPEND __DEPENDENT_OPTIONS_LATER_INVOKED_CODE "
    set(${option}_POSSIBLE 1)
    string(REGEX MATCHALL \"[^;]+\" __${option}_TOKENS \"${depends}\")
    foreach(it \${__${option}_TOKENS})
      cmake_language(EVAL CODE \"
        if (\${it})
        else()
          set(${option}_POSSIBLE 0)
        endif()\")
    endforeach()
    unset(__${option}_TOKENS)
    if(NOT ${option}_POSSIBLE)
      if(NOT \"\${${option}}\" STREQUAL \"${else}\")
        message(NOTICE \"${warn}; setting to '${else}'.\")
        set(${option} ${else} CACHE BOOL \"${doc}\" FORCE)
        set(__DEPENDENT_OPTIONS_CHANGE_HAPPENED true)
      endif()
    endif()
    unset(${option}_POSSIBLE)")
endmacro()

macro(RUN_DEPENDENCY_CHECKS)
  # controls max allowed dependency chain to avoid infinite loops during
  # configure phase
  set(__DEPENDENT_OPTIONS_LOOP_COUNTER 10)

  while(__DEPENDENT_OPTIONS_CHANGE_HAPPENED)
    MATH(EXPR __DEPENDENT_OPTIONS_LOOP_COUNTER "${__DEPENDENT_OPTIONS_LOOP_COUNTER}-1")
    set(__DEPENDENT_OPTIONS_CHANGE_HAPPENED false)
    cmake_language(EVAL CODE "${__DEPENDENT_OPTIONS_LATER_INVOKED_CODE}")
    if(__DEPENDENT_OPTIONS_LOOP_COUNTER LESS_EQUAL "0")
      break()
    endif()
  endwhile()
  unset(__DEPENDENT_OPTIONS_LOOP_COUNTER)
  set(__DEPENDENT_OPTIONS_CHANGE_HAPPENED true)
  set(__DEPENDENT_OPTIONS_LATER_INVOKED_CODE "")
endmacro()
