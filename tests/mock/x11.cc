#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "mock.hh"
#include "x11-mock.hh"

static const auto PREDEFINED_ATOMS = std::array{
    "NONE",
    "PRIMARY",
    "SECONDARY",
    "ARC",
    "ATOM",
    "BITMAP",
    "CARDINAL",
    "COLORMAP",
    "CURSOR",
    "CUT_BUFFER0",
    "CUT_BUFFER1",
    "CUT_BUFFER2",
    "CUT_BUFFER3",
    "CUT_BUFFER4",
    "CUT_BUFFER5",
    "CUT_BUFFER6",
    "CUT_BUFFER7",
    "DRAWABLE",
    "FONT",
    "INTEGER",
    "PIXMAP",
    "POINT",
    "RECTANGLE",
    "RESOURCE_MANAGER",
    "RGB_COLOR_MAP",
    "RGB_BEST_MAP",
    "RGB_BLUE_MAP",
    "RGB_DEFAULT_MAP",
    "RGB_GRAY_MAP",
    "RGB_GREEN_MAP",
    "RGB_RED_MAP",
    "STRING",
    "VISUALID",
    "WINDOW",
    "WM_COMMAND",
    "WM_HINTS",
    "WM_CLIENT_MACHINE",
    "WM_ICON_NAME",
    "WM_ICON_SIZE",
    "WM_NAME",
    "WM_NORMAL_HINTS",
    "WM_SIZE_HINTS",
    "WM_ZOOM_HINTS",
    "MIN_SPACE",
    "NORM_SPACE",
    "MAX_SPACE",
    "END_SPACE",
    "SUPERSCRIPT_X",
    "SUPERSCRIPT_Y",
    "SUBSCRIPT_X",
    "SUBSCRIPT_Y",
    "UNDERLINE_POSITION",
    "UNDERLINE_THICKNESS",
    "STRIKEOUT_ASCENT",
    "STRIKEOUT_DESCENT",
    "ITALIC_ANGLE",
    "X_HEIGHT",
    "QUAD_WIDTH",
    "WEIGHT",
    "POINT_SIZE",
    "RESOLUTION",
    "COPYRIGHT",
    "NOTICE",
    "FONT_NAME",
    "FAMILY_NAME",
    "FULL_NAME",
    "CAP_HEIGHT",
    "WM_CLASS",
    "WM_TRANSIENT_FOR",
};

static auto MOCK_ATOMS = std::vector{
    "UNKNOWN",
    "_NET_WM_STRUT",
    "_NET_WM_STRUT_PARTIAL",
};

namespace mock {
Atom name_to_atom(const char *name) {
  for (size_t i = 0; i < PREDEFINED_ATOMS.size(); i++) {
    if (std::strcmp(name, PREDEFINED_ATOMS[i]) == 0) { return i; }
  }
  for (size_t i = 1; i < MOCK_ATOMS.size(); i++) {
    if (std::strcmp(name, MOCK_ATOMS[i]) == 0) {
      return XA_LAST_PREDEFINED + i;
    }
  }
  return 0;
}
const std::string_view atom_to_name(Atom atom) {
  if (atom > XA_LAST_PREDEFINED &&
      atom - XA_LAST_PREDEFINED < MOCK_ATOMS.size()) {
    return std::string_view(MOCK_ATOMS[atom - XA_LAST_PREDEFINED]);
  } else if (atom <= XA_LAST_PREDEFINED) {
    return std::string_view(PREDEFINED_ATOMS[atom]);
  }
  return "UNKNOWN";
}

size_t format_size(std::size_t format) {
  if (format == 32) {
    return sizeof(long);
  } else if (format == 16) {
    return sizeof(short);
  } else if (format == 8) {
    return sizeof(char);
  } else {
    throw "invalid format";
  }
}
void dump_x11_blob(const std::byte *data, std::size_t format,
                   std::size_t length) {
  size_t entry_len = format_size(format);
  for (size_t i = 0; i < length * entry_len; i++) {
    if (((i + 1) % entry_len) == 1) { printf("%p: ", data + i); }
    // Print bytes in order:
    // printf("%02x ", data[i]);
    // Reorder bytes:
    printf("%02x ", (unsigned char)data[((i / entry_len - 1) * entry_len) +
                                        (2 * entry_len - 1 - (i % entry_len))]);
    if (i > 0 && ((i + 1) % entry_len) == 0) { puts(""); }
  }
  printf("Total bytes: %d\n", (int)(length * entry_len));
  puts("");
}
}  // namespace mock

extern "C" {
Atom XInternAtom(Display *display, const char *atom_name, int only_if_exists) {
  if (only_if_exists) { return mock::name_to_atom(atom_name); }
  const auto value = mock::name_to_atom(atom_name);
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
  // printf("Setting %s property data:\n", mock::atom_to_name(property).data());
  // dump_x11_blob((const std::byte *)data, format, nelements);
  mock::push_state_change(std::make_unique<mock::x11_change_property>(
      property, type, format, static_cast<mock::set_property_mode>(mode),
      (const std::byte *)data, nelements));
  return Success;
}
}
