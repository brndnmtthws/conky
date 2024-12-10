#include "mock.hh"
#include <optional>
#include <utility>

namespace mock {
std::deque<std::unique_ptr<state_change>> _state_changes;

std::deque<std::unique_ptr<state_change>> take_state_changes() {
  std::deque<std::unique_ptr<mock::state_change>> result;
  std::swap(_state_changes, result);
  return result;
}
std::optional<std::unique_ptr<state_change>> next_state_change() {
  if (_state_changes.empty()) { return std::nullopt; }
  auto front = std::move(_state_changes.front());
  _state_changes.pop_front();
  return front;
}
void push_state_change(std::unique_ptr<state_change> change) {
  _state_changes.push_back(std::move(change));
}
}  // namespace mock