#ifndef X11_MOCK_HH
#define X11_MOCK_HH

#include <string>
#include "mock.hh"

namespace mock {
struct x11_define_atom : public state_change {
  std::string name;

  x11_define_atom(std::string name) : name(name) {}

  std::string debug() {
    return debug_format("x11_define_atom { name: \"%s\" }", name.c_str());
  }
};

struct x11_change_property : public state_change {
  std::string property;
  std::string type;
  int format;
  int mode;
  const unsigned char *data;
  size_t element_count;

  x11_change_property(std::string property, std::string type, int format,
                      int mode, const unsigned char *data, size_t element_count)
      : property(property),
        type(type),
        format(format),
        mode(mode),
        data(data) {}
  std::string debug() {
    return debug_format(
        "x11_change_property { property: \"%s\", type: \"%s\", format: %d, "
        "mode: %d, data: [...], element_count: %d }",
        property.c_str(), type.c_str(), format, mode, element_count);
  }
};
}  // namespace mock

#endif /* X11_MOCK_HH */
