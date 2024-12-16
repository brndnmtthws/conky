#include "mock.hh"
#include <optional>
#include <utility>

namespace mock {
std::deque<std::unique_ptr<state_change>> __internal::state_changes;

std::deque<std::unique_ptr<state_change>> take_state_changes() {
  std::deque<std::unique_ptr<mock::state_change>> result;
  std::swap(__internal::state_changes, result);
  return result;
}
std::optional<std::unique_ptr<state_change>> next_state_change() {
  if (__internal::state_changes.empty()) { return std::nullopt; }
  auto front = std::move(__internal::state_changes.front());
  __internal::state_changes.pop_front();
  return front;
}
void push_state_change(std::unique_ptr<state_change> change) {
  __internal::state_changes.push_back(std::move(change));
}
}  // namespace mock