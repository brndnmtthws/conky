#ifndef CONKY_PARSE_VARIABLES_HH
#define CONKY_PARSE_VARIABLES_HH

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <unordered_map>

#include "content/text_object.h"
#include "macros.h"

// Forward declarations — defined in conky.cc / common.cc
int spaced_print(char *, int, const char *, int, ...);
void format_seconds(char *buf, unsigned int n, long seconds);
int extract_variable_text_internal(struct text_object *, const char *);

namespace conky::text_object {

/// Status returned by per-object constructors to let the dispatcher
/// decide error policy (log, skip, abort, fall back to plain text).
enum class create_status : uint8_t {
  success,
  unknown_variable,
  missing_argument,
  invalid_argument,
};

/// Context passed to per-object constructors by the dispatcher.
/// Constructors destructure only the fields they need.
struct construct_context {
  const char *arg;
  void **ifblock_opaque;
  void *free_at_crash;
  create_status *status;  // preset to success; overwrite on failure
};

/// Periodic data-fetch callback signature (e.g. update_cpu_usage).
using update_fn = int (*)();

/// Per-object constructor: configures an already-allocated text_object.
/// Must not allocate or free obj itself — the dispatcher owns that.
using construct_fn = void (*)(::text_object *obj,
                              const construct_context &ctx);

/// Flags encoding macro variant metadata (OBJ, OBJ_ARG, OBJ_IF, OBJ_IF_ARG).
/// Checked by the dispatcher before calling the constructor.
enum class obj_flags : uint8_t {
  plain = 0,
  arg = 1 << 0,   // arg required, fatal if null
  cond = 1 << 1,  // registers as ifblock via obj_be_ifblock_if
  // cond_arg = arg | cond
};

constexpr obj_flags operator|(obj_flags a, obj_flags b) {
  return static_cast<obj_flags>(static_cast<uint8_t>(a) |
                                static_cast<uint8_t>(b));
}

constexpr bool operator&(obj_flags a, obj_flags b) {
  return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/// Static entry in the text object lookup table.
/// One per text object name. Feature files register entries via
/// register_variable().
struct variable_definition {
  const char *name;                                 // lookup key (e.g. "cpu")
  construct_fn construct;                           // per-object constructor
  update_fn update_cb = nullptr;                    // periodic data-fetch callback
  std::chrono::milliseconds update_interval{1000};  // default 1s when update_cb set
  obj_flags flags = obj_flags::plain;               // obj_flags bitmask
};

using variable_map =
    std::unordered_map<std::string_view, variable_definition>;

/// Global registry.
const variable_map &variables();

/// Look up a variable definition by name. Returns nullptr if not found.
const variable_definition *find_variable(std::string_view name);

/// Backing function for the register_variable macro.
int register_variable_impl(std::initializer_list<variable_definition> entries);


/// Creates a variable_definition whose print callback invokes a static
/// accessor function and formats the result into the output buffer.
///
/// Template parameters:
///   Value       - return type of the accessor (deduced). Determines the
///                 printf format used to render the value. Supported types:
///                   const char* / char*        -> "%s"
///                   std::string / string_view  -> "%.*s" (precision = size)
///                   bool                       -> "yes" / "no"
///                   floating point             -> "%.2f"
///                   unsigned integral          -> "%llu"
///                   signed integral            -> "%lld"
///                   std::chrono::duration      -> format_seconds()
///                 A static_assert fires for unsupported types.
///   accessor    - a non-capturing function pointer Value(*)() that returns
///                 the current value to print. Because it is a non-type
///                 template parameter, the generated construct and print
///                 lambdas remain non-capturing and decay to function
///                 pointers — satisfying construct_fn and obj_cb::print.
///   Width       - minimum column width (default 0). When non-zero, output
///                 is padded via spaced_print() instead of snprintf().
///
/// Runtime parameters:
///   name            - the variable name used for registry lookup (e.g. "cpu")
///   update_cb       - periodic data-fetch callback, nullptr if none
///   update_interval - fetch period, default 1s (ignored when update_cb is null)
///
/// In C++17 a lambda cannot be a non-type template parameter directly.
/// The print_variable / print_variable_w macros bridge this by wrapping
/// the user-supplied lambda in a local struct's static member function,
/// whose address *is* a valid NTTP.
/// In C++20 stateless lambdas can be passed as template arguments and those
/// macros can be removed and lambdas passed directly as template arguments.
///
/// You'll likely want to use print_variable[_w] macros defined below over
/// calling this function directly.
template <typename Value, Value (*accessor)(), int Width = 0>
constexpr variable_definition print_variable_impl(const char *name,
                                   update_fn update_cb = nullptr,
                                   std::chrono::milliseconds update_interval = std::chrono::milliseconds{1000}) {
  // This function seems huge, but it produces a static data blob. Only compile times suffer :)

  constexpr bool is_duration =
      std::is_same_v<Value, std::chrono::nanoseconds> ||
      std::is_same_v<Value, std::chrono::microseconds> ||
      std::is_same_v<Value, std::chrono::milliseconds> ||
      std::is_same_v<Value, std::chrono::seconds> ||
      std::is_same_v<Value, std::chrono::minutes> ||
      std::is_same_v<Value, std::chrono::hours>;

  static_assert(std::is_same_v<Value, const char *> ||
                std::is_same_v<Value, char *> ||
                std::is_same_v<Value, bool> ||
                std::is_same_v<Value, std::string> ||
                std::is_same_v<Value, std::string_view> ||
                std::is_floating_point_v<Value> ||
                std::is_integral_v<Value> ||
                is_duration,
                "print_variable: unsupported return type from getter; instantiate variable_definition directly or extend print_variable_impl if type is general enough");

  return {name, [](::text_object *obj, const construct_context &) {
      obj->callbacks.print = [](::text_object *, char *p, unsigned int s) {
        auto value = accessor();
        if constexpr (is_duration) {
          auto secs = std::chrono::duration_cast<std::chrono::seconds>(value).count();
          format_seconds(p, s, static_cast<long>(secs));
          return;
        } else if constexpr (std::is_same_v<Value, bool>) {
          if constexpr (Width == 0) {
            snprintf(p, s, "%s", value ? "yes" : "no");
          } else {
            spaced_print(p, s, "%s", std::max(Width, 3), value ? "yes" : "no");
          }
          return;
        } else if constexpr (std::is_same_v<Value, std::string> ||
                             std::is_same_v<Value, std::string_view>) {
          if constexpr (Width == 0) {
            snprintf(p, s, "%.*s", static_cast<int>(value.size()), value.data());
          } else {
            spaced_print(p, s, "%.*s", Width, static_cast<int>(value.size()), value.data());
          }
          return;
        }
        if constexpr (std::is_same_v<Value, const char *> || std::is_same_v<Value, char *>) {
          if (value == nullptr) { *p = 0; return; }
        }
        constexpr const char *format = [] {
          if constexpr (std::is_same_v<Value, const char *> || std::is_same_v<Value, char *>) { return "%s"; }
          else if constexpr (std::is_floating_point_v<Value>) { return "%.2f"; }
          else if constexpr (std::is_unsigned_v<Value>) { return "%llu"; }
          else if constexpr (std::is_integral_v<Value>) { return "%lld"; }
          else { return ""; } // unreachable
        }();

        if constexpr (Width == 0) {
          snprintf(p, s, format, value);
        } else {
          spaced_print(p, s, format, Width, value);
        }
      };
    },
    update_cb, update_interval
  };
}

/// Print callback signature: writes the text object's output into a buffer.
using print_cb = void (*)(::text_object *, char *, unsigned int);

/// Creates a variable_definition for a variable whose argument is a live
/// sub-expression (re-evaluated each render cycle). The argument is parsed
/// into obj->sub via extract_variable_text_internal, and the given print
/// callback reads the evaluated result at render time.
///
/// Used by pid_* variables where the PID comes from a dynamic expression.
template <print_cb print_fn>
constexpr variable_definition arg_object_variable(const char *name) {
  return {name,
    [](::text_object *obj, const construct_context &ctx) {
      obj->sub = static_cast<::text_object *>(
          malloc(sizeof(struct text_object)));
      memset(obj->sub, 0, sizeof(struct text_object));
      extract_variable_text_internal(obj->sub, ctx.arg);
      obj->callbacks.print = print_fn;
    },
    nullptr, {}, obj_flags::arg};
}

::text_object *construct_text_object(const char *s, const char *arg, long line,
                                     void **ifblock_opaque,
                                     void *free_at_crash);

}  // namespace conky::text_object


/// C++17 wrapper macros for print_variable_impl - see its documentation above.
/// These bridge lambdas into NTTP function pointers via a local struct.
/// Removable once the project moves to C++20. To migrate:
///   sed -E 's/print_variable_w\(([0-9]+),\s*/print_variable<\1>(/g'
/// then delete these macros and fix up the print_variable_impl.
#define print_variable_w(width, name, lambda, ...)                              \
  ([] {                                                                         \
    struct W_ {                                                                 \
      static auto call() { return lambda(); }                                   \
    };                                                                          \
    return conky::text_object::print_variable_impl<                             \
        decltype(W_::call()), &W_::call, width>(name, ##__VA_ARGS__);           \
  })()
#define print_variable(name, lambda, ...)                                       \
  print_variable_w(0, name, lambda, ##__VA_ARGS__)

/// Each variable definition is a `variable_definition` initializer.
#define CONKY_REGISTER_VARIABLES(...)                                          \
namespace {                                                                    \
  static const auto CONKY_CONCAT(conky_reg_, __LINE__) =                       \
      conky::text_object::register_variable_impl({__VA_ARGS__});               \
}

#endif  // CONKY_PARSE_VARIABLES_HH
