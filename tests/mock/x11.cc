#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <X11/X.h>
#include <X11/Xlib.h>

#include "mock.hh"
#include "x11-mock.hh"

static auto MOCK_ATOMS = std::vector{
    "UNKNOWN",
    "_NET_WM_STRUT",
    "_NET_WM_STRUT_PARTIAL",
};

Atom name_to_atom(const char *name) {
  for (size_t i = 0; i < MOCK_ATOMS.size(); i++) {
    if (std::strcmp(name, MOCK_ATOMS[i]) == 0) { return i; }
  }
  return 0;
}
std::string atom_to_name(Atom atom) {
  if (atom < MOCK_ATOMS.size()) { return std::string(MOCK_ATOMS[atom]); }
  return "UNKNOWN";
}

extern "C" {
Atom XInternAtom(Display *display, const char *atom_name, int only_if_exists) {
  if (only_if_exists) { return name_to_atom(atom_name); }
  const auto value = name_to_atom(atom_name);
  if (value != 0) {
    return value;
  } else {
    MOCK_ATOMS.push_back(strdup(atom_name));
    mock::push_state_change(
        std::make_unique<mock::x11_define_atom>(std::string(atom_name)));
    return MOCK_ATOMS.size() - 1;
  }
}

int XChangeProperty(Display *display, Window w, Atom property, Atom type,
                    int format, int mode, const unsigned char *data,
                    int nelements) {
  mock::push_state_change(std::make_unique<mock::x11_change_property>(
      atom_to_name(property), atom_to_name(type), format, mode, data,
      nelements));
  return Success;
}
}
