#ifndef _CONKY_MACROS_H_
#define _CONKY_MACROS_H_

#include "logging.h"

// LIKELY and UNLIKELY can be used to mark if statement branches as likely (hot)
// or unlikely (cold). They help compiler rearange code to optimize hot paths
// and can improve perfomance when used on architectures with support for branch
// prediction.

#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (!!(x))
#define UNLIKELY(x) (!!(x))
#endif  // defined(__clang__) || defined(__GNUC__)

// Assumptions are contracts which are expected to be upheld by the developer
// 100% of the time. They help the compiler significantly optimize code, but
// cause UB if not upheld. As such, they are not a substitute for proper error
// handling or assertions.
//
// They throw CRIT_ERR in debug builds.

#ifdef NDEBUG
#ifdef _MSC_VER
#define ASSUME(cond) __assume(cond)
#elif defined(__clang__)
#define ASSUME(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
#define ASSUME(cond) ((cond) ? static_cast<void>(0) : __builtin_unreachable())
#else /* other compilers */
#define ASSUME(cond) static_cast<void>(!!(cond))
#endif /* compiler selection */
#else  /* DEBUG */
#define ASSUME(cond) \
  (LIKELY(cond) ? static_cast<void>(0) : CRIT_ERR("assertion " #cond " failed"))
#endif /* NDEBUG */

// UNREACHABLE is used to mark unreachable part of code execution which helps
// the compiler optimize code, but also causes UB if marked position in code is
// actually reached.
//
// It throws CRIT_ERR in debug builds.

#ifdef NDEBUG
#ifdef __GNUC__  // GCC, Clang, ICC
#define UNREACHABLE() (__builtin_unreachable())
#elif defined(_MSC_VER)  // MSVC
#define UNREACHABLE() (__assume(false))
#else /* other compilers */
// unreachable_impl must be emitted in a separated TU if used from C code due to
// the difference in rule for inline functions in C. Conky is compiled by a C++
// compiler though so it's fine to place it here.
[[noreturn]] inline void unreachable_impl() {}
#define UNREACHABLE() (unreachable_impl())
#endif /* compiler selection */
#else  /* DEBUG */
#define UNREACHABLE() (CRIT_ERR("reached unreachable"))
#endif /* NDEBUG */

#endif /* _CONKY_MACROS_H_ */