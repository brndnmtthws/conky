# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
DependentOption
--------------------
Roughly based on CMakeDependentOption:
https://github.com/Kitware/CMake/blob/master/Modules/CMakeDependentOption.cmake

Modified to so it produces warnings instead of hiding an option completely and
sets a default value.

Argument meaning and order are the same, and there's an additional (warn)
argument which is the message printed if the end-user enabled a feature which
isn't "possible".
#]=======================================================================]

macro(DEPENDENT_OPTION option doc default depends else warn)
  set(${option}_POSSIBLE 1)
  foreach(d ${depends})
    cmake_language(EVAL CODE "
        if (${d})
        else()
          set(${option}_POSSIBLE 0)
        endif()"
    )
  endforeach()
  option(${option} "${doc}" "${default}")
  if(NOT ${option}_POSSIBLE)
    if(NOT ${option} MATCHES ${else})
      message(NOTICE "${warn}; setting to '${else}'.")
    endif()
    set(${option} ${else} CACHE BOOL "${doc}" FORCE)
  endif()
  unset(${option}_POSSIBLE)
endmacro()
