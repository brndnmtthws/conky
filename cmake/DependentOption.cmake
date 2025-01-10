# Distributed under the MIT/Apache 2.0/zlib License.
# Author: Tin Švagelj (Caellian) <tin.svagelj@live.com>

#[[.md:
# DependentOption

```cmake
dependent_option(
  <option> "<help_text>" [<initial_value>]
  [DEPENDS <conditions>]
  [FALLBACK <value>]
  [SHOW_IF <conditions>]
  [TYPE (BOOL|FILE|DIR|STRING|INT|FLOAT)]
  [WARN "<warning>"]
)
```

`DEPENDS <conditions>` argument forces the value of the variable value to be
the fallback value.

`SHOW_IF <conditions>` argument allows hiding the option like
`CMakeDependentOption` does by default if provided `<conditions>` aren't met.
If it's not specified then then the option is always shown.

`WARN "<warning>"` allows specifying a custom `<warning>` message if `DEPENDS`
conditions aren't satisfied and current variable value does not match the
fallback value. Default is inferred from other parameters.

`FALLBACK` allows customizing the fallback value, in case `<initial_value>` is
not the value that should be used in case `DEPENDS` or `HIDE_IF` aren't
satisfied.

`TYPE` allows specifying option value type. This affects how the option is
presented in `ccmake` and `cmake-gui`, as well as default value if neither.
Types `INT` and `FLOAT` are mapped to `STRING`, but `INT` value is permitted to
simplify composition with other macros that might need to have special handling
for numeric values.

If neither `FALLBACK` nor `<initial_value>` are specified the both initial and
fallback value will be:
- `OFF` if variable type is `BOOL`,
- an empty string if type is `FILE`, `DIR`, or `STRING`,
- `0` if variable type is `INT` or `FLOAT`,
- `OFF` if no `TYPE` has been specified.

`<conditions>` are a ;-separated list of boolean expressions. Any in scope
value can be used, not just other options, so long as the expression itself
doesn't require `;` characters. In those cases it should be stored in a
separate variable and that variable should be used instead.

All checks are deferred until `RUN_DEPENDENCY_CHECKS` macro is called in order to
allow out-of-order declaration of dependencies and handle dependecy graph cycles.
As the checks can affect each other they're run in a loop until the graph settles.

See: https://github.com/Kitware/CMake/blob/master/Modules/CMakeDependentOption.cmake
]]

set(__DEPENDENT_OPTIONS_CHANGE_HAPPENED true)
set(__DEPENDENT_OPTIONS_LATER_INVOKED_CODE "")

macro(__DEPENDENT_OPTIONS_TYPE_MAP TYPE_NAME)
  if("${${TYPE_NAME}}" STREQUAL "FILE")
    set("${TYPE_NAME}" "FILEPATH")
  elseif("${${TYPE_NAME}}" STREQUAL "DIR")
    set("${TYPE_NAME}" "PATH")
  elseif("${${TYPE_NAME}}" STREQUAL "INT")
    set("${TYPE_NAME}" "STRING")
  elseif("${${TYPE_NAME}}" STREQUAL "FLOAT")
    set("${TYPE_NAME}" "STRING")
  endif()
  # else: do nothing; assume a valid value is provided
endmacro()
macro(__DEPENDENT_OPTIONS_TYPE_DEFAULT TYPE_NAME VARIABLE_OUT)
  if("${${TYPE_NAME}}" STREQUAL "BOOL")
    set("${VARIABLE_OUT}" "OFF")
  elseif("${${TYPE_NAME}}" MATCHES "(FILE|DIR|STRING|INTERNAL)")
    set("${VARIABLE_OUT}" "\"\"")
  elseif("${${TYPE_NAME}}" STREQUAL "INT")
    set("${VARIABLE_OUT}" "0")
  elseif("${${TYPE_NAME}}" STREQUAL "FLOAT")
    set("${VARIABLE_OUT}" "0")
  else()
    # default for OPTION function
    set("${VARIABLE_OUT}" "OFF")
  endif()
endmacro()

function(DEPENDENT_OPTION option doc)
  cmake_parse_arguments(PARSE_ARGV 2 arg
    "" "FALLBACK WARN TYPE" "DEPENDS SHOW_IF"
  )
  list(LENGTH arg_UNPARSED_ARGUMENTS POSITIONAL_COUNT)
  if(POSITIONAL_COUNT GREATER 0)
    list(POP_FRONT arg_UNPARSED_ARGUMENTS INITIAL_VALUE)
    math(EXPR POSITIONAL_COUNT "${POSITIONAL_COUNT}-1")
  endif()

  set(WARNING_MESSAGE "${option} can't be enabled.")
  set(DYN_CHECK)
  if(DEFINED arg_DEPENDS)
    set(WARNING_MESSAGE "${option} requires ${arg_DEPENDS}.")
    set(DYN_CHECK "
    string(REGEX MATCHALL \"[^;]+\" __${option}_TOKENS \"${arg_DEPENDS}\")
    foreach(it \${__${option}_TOKENS})
      cmake_language(EVAL CODE \"
        if (\${it})
        else()
          set(${option}_POSSIBLE 0)
        endif()\")
    endforeach()
    unset(__${option}_TOKENS)")
  endif()

  set(OPTION_TYPE "BOOL")
  if(DEFINED arg_TYPE)
    __dependent_options_type_map(arg_TYPE)
    set(OPTION_TYPE "${arg_TYPE}")
  endif()

  if(DEFINED arg_FALLBACK)
    if(NOT DEFINED INITIAL_VALUE)
      set(INITIAL_VALUE "${arg_FALLBACK}")
    endif()
  else()
    if(DEFINED INITIAL_VALUE)
      set(arg_FALLBACK INITIAL_VALUE)
    else()
      __dependent_options_type_default("${arg_TYPE}" INITIAL_VALUE)
      __dependent_options_type_default("${arg_TYPE}" arg_FALLBACK)
    endif()
  endif()
  
  if(DEFINED arg_SHOW_IF)
    set("__${option}_VISIBLE" 0)
    string(REGEX MATCHALL "[^;]+" "__${option}_HIDE_TOKENS" "${arg_SHOW_IF}")
    foreach(it "${__${option}_HIDE_TOKENS}")
      cmake_language(EVAL CODE "
        if (${it})
        else()
          set(__${option}_VISIBLE 1)
        endif()")
    endforeach()
  endif()
  if(DEFINED arg_WARN)
    set(WARNING_MESSAGE "${arg_WARN}")
  endif()

  if(NOT DEFINED "__${option}_VISIBLE" OR "${__${option}_VISIBLE}")
    set(${option} "${INITIAL_VALUE}" CACHE "${OPTION_TYPE}" "${doc}")
  else()
    set(${option} "${arg_FALLBACK}" CACHE INTERNAL "${doc}")
    return()
  endif()

  if(DEFINED arg_DEPENDS)
    string(APPEND __DEPENDENT_OPTIONS_LATER_INVOKED_CODE "
      set(${option}_POSSIBLE 1)
      ${DYN_CHECK}
      if(NOT ${option}_POSSIBLE)
        if(NOT \"\${${option}}\" STREQUAL \"${arg_FALLBACK}\")
          if(NOT \"\${${option}}\" STREQUAL \"${INITIAL_VALUE}\")
            message(WARNING \"${WARNING_MESSAGE}; setting to '${__DEPENDENT_OPTIONS_FALLBACK}'.\")
          endif()
          set(${option} ${__DEPENDENT_OPTIONS_FALLBACK})
          set(__DEPENDENT_OPTIONS_CHANGE_HAPPENED true)
        endif()
      endif()
      unset(${option}_POSSIBLE)")
    set(__DEPENDENT_OPTIONS_LATER_INVOKED_CODE "${__DEPENDENT_OPTIONS_LATER_INVOKED_CODE}" PARENT_SCOPE)
  endif()
endfunction()

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
