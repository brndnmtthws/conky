#include "catch2/catch.hpp"

#include "config.h"

#ifdef BUILD_X11
#include <string>

#include "conky.h"
#include "output/x11.h"

namespace {
struct desktop_test_context {
  desktop_info saved_desktop = info.x11.desktop;

  ~desktop_test_context() { info.x11.desktop = saved_desktop; }

  void check(const std::string &names, int current,
             const std::string &expected) {
    CAPTURE(current, names.size());
    info.x11.desktop.current = current;
    info.x11.desktop.name = "previous";
    get_x11_desktop_current_name(names);
    CHECK(info.x11.desktop.name == expected);
  }
};
}  // namespace

TEST_CASE("x11_desktop_names", "[x11][x11_desktop]") {
  using namespace std::string_literals;
  desktop_test_context context;

  SECTION("last_name_with_or_without_terminator") {
    for (const auto &names :
         {"first\0second\0last"s, "first\0second\0last\0"s}) {
      context.check(names, 1, "first");
      context.check(names, 2, "second");
      context.check(names, 3, "last");
    }
  }

  SECTION("single_desktop") {
    context.check("only", 1, "only");
    context.check("only\0"s, 1, "only");
  }

  SECTION("empty_names_do_not_shift_indices") {
    context.check("\0middle\0last"s, 1, "");
    context.check("\0middle\0last"s, 2, "middle");
    context.check("first\0\0last"s, 2, "");
    context.check("first\0\0last"s, 3, "last");
    context.check("first\0\0"s, 2, "");
  }

  SECTION("utf8_names") {
    context.check("one\0二\0три"s, 2, "二");
    context.check("one\0二\0три"s, 3, "три");
  }

  SECTION("missing_or_invalid_desktop_clears_stale_name") {
    context.check("", 1, "");
    context.check("first\0last"s, 0, "");
    context.check("first\0last"s, -1, "");
    context.check("first\0last"s, 3, "");
    context.check("first\0last\0"s, 3, "");
    context.check("shorter list", 2, "");
  }
}
#endif
