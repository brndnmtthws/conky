#ifndef X11_MOCK_HH
#define X11_MOCK_HH

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "mock.hh"

namespace mock {

enum x11_property_type {
  ARC = XA_ARC,
  ATOM = XA_ATOM,
  BITMAP = XA_BITMAP,
  CARDINAL = XA_CARDINAL,
  COLORMAP = XA_COLORMAP,
  CURSOR = XA_CURSOR,
  DRAWABLE = XA_DRAWABLE,
  FONT = XA_FONT,
  INTEGER = XA_INTEGER,
  PIXMAP = XA_PIXMAP,
  POINT = XA_POINT,
  RGB_COLOR_MAP = XA_RGB_COLOR_MAP,
  RECTANGLE = XA_RECTANGLE,
  STRING = XA_STRING,
  VISUALID = XA_VISUALID,
  WINDOW = XA_WINDOW,
  WM_HINTS = XA_WM_HINTS,
  WM_SIZE_HINTS = XA_WM_SIZE_HINTS,
};

Atom name_to_atom(const char *name);
const std::string_view atom_to_name(Atom atom);
size_t format_size(std::size_t format);
void dump_x11_blob(const std::byte *data, std::size_t format,
                   std::size_t length);

/// Mutation produced by creating new `Atom`s.
struct x11_define_atom : public state_change {
  std::string name;

  x11_define_atom(std::string name) : name(name) {}

  static std::string change_name() { return "x11_define_atom"; }

  std::string debug() {
    return debug_format("x11_define_atom { name: \"%s\" }", name.c_str());
  }
};

enum class set_property_mode {
  REPLACE = PropModeReplace,
  PREPEND = PropModePrepend,
  APPEND = PropModeAppend,
};

/// Mutation produced by calls to XChangeProperty.
class x11_change_property : public state_change {
  Atom m_property;
  Atom m_type;
  std::size_t m_format;
  set_property_mode m_mode;
  std::vector<std::byte> m_data;
  std::size_t m_element_count;

 public:
  x11_change_property(Atom property, Atom type, std::size_t format,
                      set_property_mode mode, const std::byte *data,
                      std::size_t element_count)
      : m_property(property),
        m_type(type),
        m_format(format),
        m_mode(mode),
        m_element_count(element_count),
        m_data(std::vector(data, data + format_size(format) * element_count)) {}

  static std::string change_name() { return "x11_change_property"; }

  Atom property() const { return m_property; }
  std::string_view property_name() const { return atom_to_name(m_property); }
  Atom type() const { return m_type; }
  std::string_view type_name() const { return atom_to_name(m_type); }
  std::size_t format() const { return m_format; }
  set_property_mode mode() const { return m_mode; }
  std::string_view mode_name() const {
    switch (m_mode) {
      case mock::set_property_mode::REPLACE:
        return "replace";
      case mock::set_property_mode::PREPEND:
        return "prepend";
      case mock::set_property_mode::APPEND:
        return "append";
      default:
        return "other";
    }
  }
  std::size_t element_count() const { return m_element_count; }
  const std::byte *data() const { return m_data.data(); }

  std::string debug() {
    return debug_format(
        "x11_change_property { property: \"%s\", type: \"%s\", format: %d, "
        "mode: %s (%d), data: [...], element_count: %d }",
        property_name(), type_name(), m_format, mode_name(), m_mode,
        m_element_count);
  }
};
}  // namespace mock

#define REQUIRE_FORMAT_SIZE(format, T) REQUIRE(format == (sizeof(T) * 8))

// These are only macros because including Catch2 from mocking causes spurious
// errors. I whish they weren't because they're such a pain to write this way.

// Originally a single templated function:
//
// template <typename D, const std::size_t Count>
// const D &expect_x11_data(
//   const std::byte* data, Atom type, std::size_t format,
//   std::size_t element_count
// ) {...}
//
// It is a somewhat large blob, but most of it will be compiled away. The only
// downside is that lambdas must return owned values.

#define EXPECT_X11_VALUE(data, type, format, element_count, T)                 \
  []() {                                                                       \
    if constexpr (std::is_same_v<XID, std::uint32_t> &&                        \
                  std::is_same_v<T, std::uint32_t>) {                          \
      if (!(type == mock::x11_property_type::ATOM ||                           \
            type == mock::x11_property_type::BITMAP ||                         \
            type == mock::x11_property_type::CARDINAL ||                       \
            type == mock::x11_property_type::PIXMAP ||                         \
            type == mock::x11_property_type::COLORMAP ||                       \
            type == mock::x11_property_type::CURSOR ||                         \
            type == mock::x11_property_type::DRAWABLE ||                       \
            type == mock::x11_property_type::FONT ||                           \
            type == mock::x11_property_type::VISUALID ||                       \
            type == mock::x11_property_type::WINDOW)) {                        \
        FAIL(                                                                  \
            "expected unsigned long data; got: " << mock::atom_to_name(type)); \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, std::uint32_t);                              \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const std::uint32_t *>(data);                   \
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {                   \
      if (type != mock::x11_property_type::CARDINAL) {                         \
        FAIL("expected CARDINAL data; got: " << mock::atom_to_name(type));     \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, std::uint32_t);                              \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const std::uint32_t *>(data);                   \
    } else if constexpr (std::is_same_v<T, XID>) {                             \
      if (!(type == mock::x11_property_type::ATOM ||                           \
            type == mock::x11_property_type::BITMAP ||                         \
            type == mock::x11_property_type::PIXMAP ||                         \
            type == mock::x11_property_type::COLORMAP ||                       \
            type == mock::x11_property_type::CURSOR ||                         \
            type == mock::x11_property_type::DRAWABLE ||                       \
            type == mock::x11_property_type::FONT ||                           \
            type == mock::x11_property_type::VISUALID ||                       \
            type == mock::x11_property_type::WINDOW)) {                        \
        FAIL("expected XID data; got: " << mock::atom_to_name(type));          \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, XID);                                        \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const XID *>(data);                             \
    } else if constexpr (std::is_same_v<T, std::int32_t>) {                    \
      if (type != mock::x11_property_type::INTEGER) {                          \
        FAIL("expected INTEGER data; got: " << mock::atom_to_name(type));      \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, std::int32_t);                               \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const std::int32_t *>(data);                    \
    } else if constexpr (std::is_same_v<T, XRectangle>) {                      \
      if (type != mock::x11_property_type::RECTANGLE) {                        \
        FAIL("expected RECTANGLE data; got: " << mock::atom_to_name(type));    \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, short);                                      \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const XRectangle *>(data);                      \
    } else if constexpr (std::is_same_v<T, XArc>) {                            \
      if (type != mock::x11_property_type::ARC) {                              \
        FAIL("expected ARC data; got: " << mock::atom_to_name(type));          \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, short);                                      \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const XArc *>(data);                            \
    } else if constexpr (std::is_same_v<T, XColor>) {                          \
      if (type != mock::x11_property_type::RGB_COLOR_MAP) {                    \
        FAIL(                                                                  \
            "expected RGB_COLOR_MAP data; got: " << mock::atom_to_name(type)); \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, short);                                      \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const XColor *>(data);                          \
    } else if constexpr (std::is_same_v<T, std::string_view> ||                \
                         std::is_same_v<T, std::string>) {                     \
      if (type != mock::x11_property_type::STRING) {                           \
        FAIL("expected STRING data; got: " << mock::atom_to_name(type));       \
      }                                                                        \
      REQUIRE_FORMAT_SIZE(format, char);                                       \
      return T(reinterpret_cast<const char *>(data), element_count);           \
    } else if constexpr (std::is_same_v<T, XWMHints>) {                        \
      if (type != mock::x11_property_type::WM_HINTS) {                         \
        FAIL("expected WM_HINTS data; got: " << mock::atom_to_name(type));     \
      }                                                                        \
      /* TODO: Not sure: REQUIRE_FORMAT_SIZE(format, unsigned long); */        \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const XWMHints *>(data);                        \
    } else if constexpr (std::is_same_v<T, XSizeHints>) {                      \
      if (type != mock::x11_property_type::WM_SIZE_HINTS) {                    \
        FAIL(                                                                  \
            "expected WM_SIZE_HINTS data; got: " << mock::atom_to_name(type)); \
      }                                                                        \
      /* TODO: Not sure: REQUIRE_FORMAT_SIZE(format, unsigned long); */        \
      REQUIRE(element_count == 1);                                             \
      return *reinterpret_cast<const XSizeHints *>(data);                      \
    } else {                                                                   \
      throw "unimplemented conversion"                                         \
    }                                                                          \
  }()

#define _COPY_C_ARRAY_TO_CAST(BaseT, TargetT, Length, source)    \
  [&]() {                                                        \
    auto values = reinterpret_cast<const BaseT *>(source);       \
    auto result = std::array<TargetT, Length>{};                 \
    for (size_t i = 0; i < Length; i++) {                        \
      if constexpr (std::numeric_limits<BaseT>::max() >          \
                    std::numeric_limits<TargetT>::max()) {       \
        CHECK(values[i] >= std::numeric_limits<TargetT>::min()); \
        CHECK(values[i] <= std::numeric_limits<TargetT>::max()); \
      }                                                          \
      result[i] = static_cast<TargetT>(values[i]);               \
    }                                                            \
    return result;                                               \
  }()

#define EXPECT_X11_ARRAY(data, type, format, element_count, T, Count)       \
  [&]() {                                                                   \
    if constexpr (std::is_same_v<XID, std::uint32_t> &&                     \
                  std::is_same_v<T, std::uint32_t>) {                       \
      if (!(type == mock::x11_property_type::ATOM ||                        \
            type == mock::x11_property_type::BITMAP ||                      \
            type == mock::x11_property_type::CARDINAL ||                    \
            type == mock::x11_property_type::PIXMAP ||                      \
            type == mock::x11_property_type::COLORMAP ||                    \
            type == mock::x11_property_type::CURSOR ||                      \
            type == mock::x11_property_type::DRAWABLE ||                    \
            type == mock::x11_property_type::FONT ||                        \
            type == mock::x11_property_type::VISUALID ||                    \
            type == mock::x11_property_type::WINDOW)) {                     \
        FAIL("expected unsigned long array; got: "                          \
             << mock::atom_to_name(type));                                  \
      }                                                                     \
      REQUIRE_FORMAT_SIZE(format, std::uint32_t);                           \
      REQUIRE(element_count == Count);                                      \
      return _COPY_C_ARRAY_TO_CAST(long, std::uint32_t, Count, data);       \
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {                \
      if (type != mock::x11_property_type::CARDINAL) {                      \
        FAIL("expected CARDINAL array; got: " << mock::atom_to_name(type)); \
      }                                                                     \
      REQUIRE_FORMAT_SIZE(format, std::uint32_t);                           \
      REQUIRE(element_count == Count);                                      \
      return _COPY_C_ARRAY_TO_CAST(long, std::uint32_t, Count, data);       \
    } else if constexpr (std::is_same_v<T, XID>) {                          \
      if (!(type == mock::x11_property_type::ATOM ||                        \
            type == mock::x11_property_type::BITMAP ||                      \
            type == mock::x11_property_type::PIXMAP ||                      \
            type == mock::x11_property_type::COLORMAP ||                    \
            type == mock::x11_property_type::CURSOR ||                      \
            type == mock::x11_property_type::DRAWABLE ||                    \
            type == mock::x11_property_type::FONT ||                        \
            type == mock::x11_property_type::VISUALID ||                    \
            type == mock::x11_property_type::WINDOW)) {                     \
        FAIL("expected XID data; got: " << mock::atom_to_name(type));       \
      }                                                                     \
      REQUIRE_FORMAT_SIZE(format, XID);                                     \
      REQUIRE(element_count == Count);                                      \
      return _COPY_C_ARRAY_TO_CAST(long, T, Count, data);                   \
    } else if constexpr (std::is_same_v<T, std::int32_t>) {                 \
      if (type != mock::x11_property_type::INTEGER) {                       \
        FAIL("expected INTEGER array; got: " << mock::atom_to_name(type));  \
      }                                                                     \
      REQUIRE_FORMAT_SIZE(format, std::int32_t);                            \
      REQUIRE(element_count == Count);                                      \
      return _COPY_C_ARRAY_TO_CAST(long, std::int32_t, Count, data);        \
    } else {                                                                \
      throw "unimplemented conversion";                                     \
    }                                                                       \
  }()

#define _COPY_C_ARRAY_TO_VEC(BaseT, TargetT, source, length)     \
  [&]() {                                                        \
    auto values = reinterpret_cast<const BaseT *>(source);       \
    auto result = std::vector<TargetT>(length);                  \
    for (const BaseT *it = values; it < values + length; it++) { \
      if constexpr (std::numeric_limits<BaseT>::max() >          \
                    std::numeric_limits<TargetT>::max()) {       \
        CHECK(*it >= std::numeric_limits<TargetT>::min());       \
        CHECK(*it <= std::numeric_limits<TargetT>::max());       \
      }                                                          \
      result.push_back(*it);                                     \
    }                                                            \
    return result;                                               \
  }()

#define EXPECT_X11_VEC(data, type, format, element_count, T)                 \
  [&]() {                                                                    \
    if constexpr (std::is_same_v<XID, std::uint32_t> &&                      \
                  std::is_same_v<T, std::uint32_t>) {                        \
      if (!(type == mock::x11_property_type::ATOM ||                         \
            type == mock::x11_property_type::BITMAP ||                       \
            type == mock::x11_property_type::CARDINAL ||                     \
            type == mock::x11_property_type::PIXMAP ||                       \
            type == mock::x11_property_type::COLORMAP ||                     \
            type == mock::x11_property_type::CURSOR ||                       \
            type == mock::x11_property_type::DRAWABLE ||                     \
            type == mock::x11_property_type::FONT ||                         \
            type == mock::x11_property_type::VISUALID ||                     \
            type == mock::x11_property_type::WINDOW)) {                      \
        FAIL("expected unsigned long array; got: "                           \
             << mock::atom_to_name(type));                                   \
      }                                                                      \
      REQUIRE_FORMAT_SIZE(format, std::uint32_t);                            \
      return _COPY_C_ARRAY_TO_VEC(long, std::uint32_t, data, element_count); \
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {                 \
      if (type != mock::x11_property_type::CARDINAL) {                       \
        FAIL("expected CARDINAL array; got: " << mock::atom_to_name(type));  \
      }                                                                      \
      REQUIRE_FORMAT_SIZE(format, std::uint32_t);                            \
      return _COPY_C_ARRAY_TO_VEC(long, std::uint32_t, data, element_count); \
    } else if constexpr (std::is_same_v<T, XID>) {                           \
      if (!(type == mock::x11_property_type::ATOM ||                         \
            type == mock::x11_property_type::BITMAP ||                       \
            type == mock::x11_property_type::PIXMAP ||                       \
            type == mock::x11_property_type::COLORMAP ||                     \
            type == mock::x11_property_type::CURSOR ||                       \
            type == mock::x11_property_type::DRAWABLE ||                     \
            type == mock::x11_property_type::FONT ||                         \
            type == mock::x11_property_type::VISUALID ||                     \
            type == mock::x11_property_type::WINDOW)) {                      \
        FAIL("expected XID data; got: " << mock::atom_to_name(type));        \
      }                                                                      \
      REQUIRE_FORMAT_SIZE(format, XID);                                      \
      return _COPY_C_ARRAY_TO_VEC(long, XID, data, element_count);           \
    } else if constexpr (std::is_same_v<T, std::int32_t>) {                  \
      if (type != mock::x11_property_type::INTEGER) {                        \
        FAIL("expected INTEGER array; got: " << mock::atom_to_name(type));   \
      }                                                                      \
      REQUIRE_FORMAT_SIZE(format, std::int32_t);                             \
      return _COPY_C_ARRAY_TO_VEC(long, std::int32_t, data, element_count);  \
    } else {                                                                 \
      throw "unimplemented conversion";                                      \
    }                                                                        \
  }()

#endif /* X11_MOCK_HH */
