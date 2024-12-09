#ifndef MOCK_HH
#define MOCK_HH

#include <cstdio>
#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace mock {

/// Ponyfill for `std::format`.
template <typename... Args>
std::string debug_format(const std::string& format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
               1;  // Extra space for '\0'
  if (size_s <= 0) { throw std::runtime_error("error during formatting"); }
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1);  // We don't want the '\0' inside
}

/// Base class of state changes.
///
/// A state change represents some side effect that mutates system state via
/// mocked functions.
///
/// For directly accessible globals and fields, those should be used instead.
/// This is intended for cases where some library function is internally invoked
/// but would fail if conditions only present at runtime aren't met.
struct state_change {
 public:
  virtual ~state_change() = default;
  /// Returns a string representation of this state change with information
  /// necessary to differentiate it from other variants of the same type.
  virtual std::string debug() = 0;
};

/// Implementation detail; shouldn't be used directly.
extern std::deque<std::unique_ptr<state_change>> _state_changes;

/// Removes all `state_change`s from the queue (clearing it) and returns them.
std::deque<std::unique_ptr<state_change>> take_state_changes();

/// Pops the next `state_change` from the queue, or returns `std::nullopt` if
/// there are none.
std::optional<std::unique_ptr<state_change>> next_state_change();

/// Pushes some `state_change` to the back of the queue.
void push_state_change(std::unique_ptr<state_change> change);

/// Pops some `state_change` of type `T` if it's the next change in the queue.
/// Otherwise it returns `std::nullopt`.
template <typename T>
std::optional<T> next_state_change_t() {
  static_assert(std::is_base_of_v<state_change, T>, "T must be a state_change");
  auto result = next_state_change();
  if (!result.has_value()) { return std::nullopt; }
  auto cast_result = dynamic_cast<T*>(result.value().get());
  if (!cast_result) {
    _state_changes.push_front(std::move(result.value()));
    return std::nullopt;
  }
  return *dynamic_cast<T*>(result.value().release());
}
}  // namespace mock

/// A variant of `mock::next_state_change_t` that integrates into Catch2.
/// It's a macro because using `FAIL` outside of a test doesn't work.
#define EXPECT_NEXT_CHANGE(T)                                  \
  []() {                                                       \
    static_assert(std::is_base_of_v<mock::state_change, T>,    \
                  #T " isn't a state_change");                 \
    auto result = mock::next_state_change();                   \
    if (!result.has_value()) {                                 \
      FAIL("no more state changes; expected '" #T "'");        \
      return *reinterpret_cast<T*>(malloc(sizeof(T)));         \
    }                                                          \
    auto cast_result = dynamic_cast<T*>(result.value().get()); \
    if (!cast_result) {                                        \
      FAIL("expected '" #T "' as next state change, got: "     \
           << result.value().get()->debug());                  \
      return *reinterpret_cast<T*>(malloc(sizeof(T)));         \
    } else {                                                   \
      return *dynamic_cast<T*>(result.value().release());      \
    }                                                          \
  }();
// garbage reinterpretation after FAIL doesn't get returned because FAIL stops
// the test. Should be UNREACHABLE, but I have trouble including it.

#endif /* MOCK_HH */
