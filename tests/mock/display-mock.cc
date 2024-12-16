#include "display-mock.hh"
#include "display-output.hh"

namespace mock {
display_output_mock *output;

display_output_mock &get_mock_output() { return *output; }

namespace __internal {
void init_display_output_mock() {
  output = new display_output_mock();
  conky::active_display_outputs.push_back(output);
  conky::current_display_outputs.push_back(output);
}
void delete_display_output_mock() {
  delete output;
  conky::current_display_outputs.clear();
  conky::active_display_outputs.clear();
}
}  // namespace __internal
}  // namespace mock