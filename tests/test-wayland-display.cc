#include "catch2/catch.hpp"
#include "config.h"

#ifdef BUILD_WAYLAND
#include <memory>
#include <string>
#include <vector>

#include "output/wayland-display.hh"

namespace {
struct fake_output {
  std::string name;
  std::string description;
};
}  // namespace

TEST_CASE("wayland_display_parse", "[wayland][display]") {
  SECTION("empty string selects default socket and compositor default output") {
    auto spec = conky::parse_wayland_display("");
    CHECK(spec.socket_name.empty());
    CHECK(spec.output_selector.empty());
  }

  SECTION("plain output name uses default socket") {
    auto spec = conky::parse_wayland_display("DP-2");
    CHECK(spec.socket_name.empty());
    CHECK(spec.output_selector == "DP-2");
  }

  SECTION("description containing colon is not mistaken for X11 display") {
    auto spec = conky::parse_wayland_display("Dell Inc.: U2720Q");
    CHECK(spec.socket_name.empty());
    CHECK(spec.output_selector == "Dell Inc.: U2720Q");
  }

  SECTION("plain ordinal index uses default socket") {
    auto spec = conky::parse_wayland_display("1");
    CHECK(spec.socket_name.empty());
    CHECK(spec.output_selector == "1");
  }

  SECTION("wayland socket only") {
    auto spec = conky::parse_wayland_display("wayland-1");
    CHECK(spec.socket_name == "wayland-1");
    CHECK(spec.output_selector.empty());
  }

  SECTION("wayland socket with ordinal or named output") {
    auto by_idx = conky::parse_wayland_display("wayland-0.2");
    CHECK(by_idx.socket_name == "wayland-0");
    CHECK(by_idx.output_selector == "2");

    auto by_name = conky::parse_wayland_display("wayland-0.DP-2");
    CHECK(by_name.socket_name == "wayland-0");
    CHECK(by_name.output_selector == "DP-2");

    auto by_desc = conky::parse_wayland_display("wayland-0.Dell Inc. U2415");
    CHECK(by_desc.socket_name == "wayland-0");
    CHECK(by_desc.output_selector == "Dell Inc. U2415");
  }

  SECTION("X11-style display syntax maps to wayland socket and output") {
    auto sock_only = conky::parse_wayland_display(":0");
    CHECK(sock_only.socket_name == "wayland-0");
    CHECK(sock_only.output_selector.empty());

    auto with_screen = conky::parse_wayland_display(":1.2");
    CHECK(with_screen.socket_name == "wayland-1");
    CHECK(with_screen.output_selector == "2");

    auto with_name = conky::parse_wayland_display(":0.DP-2");
    CHECK(with_name.socket_name == "wayland-0");
    CHECK(with_name.output_selector == "DP-2");
  }

  SECTION("absolute socket path is preserved as socket_name") {
    auto abs = conky::parse_wayland_display("/run/user/1000/wayland-0");
    CHECK(abs.socket_name == "/run/user/1000/wayland-0");
    CHECK(abs.output_selector.empty());

    auto abs_dot = conky::parse_wayland_display("/tmp/custom.sock");
    CHECK(abs_dot.socket_name == "/tmp/custom.sock");
    CHECK(abs_dot.output_selector.empty());
  }

  SECTION("non-numeric suffixes after wayland- or : fall back to selector") {
    auto custom = conky::parse_wayland_display("wayland-custom");
    CHECK(custom.socket_name.empty());
    CHECK(custom.output_selector == "wayland-custom");

    auto invalid_colon = conky::parse_wayland_display(":invalid");
    CHECK(invalid_colon.socket_name.empty());
    CHECK(invalid_colon.output_selector == ":invalid");
  }

  SECTION("trailing dot leaves output selector empty") {
    auto wl_dot = conky::parse_wayland_display("wayland-0.");
    CHECK(wl_dot.socket_name == "wayland-0");
    CHECK(wl_dot.output_selector.empty());

    auto x_dot = conky::parse_wayland_display(":0.");
    CHECK(x_dot.socket_name == "wayland-0");
    CHECK(x_dot.output_selector.empty());
  }
}

TEST_CASE("wayland_output_selection", "[wayland][display]") {
  std::vector<std::unique_ptr<fake_output>> outputs;
  outputs.push_back(
      std::make_unique<fake_output>(fake_output{"DP-3", "LG OLED 4K"}));
  outputs.push_back(
      std::make_unique<fake_output>(fake_output{"DP-2", "Dell U2415"}));
  outputs.push_back(
      std::make_unique<fake_output>(fake_output{"DP-1", "ASUS PA278QV"}));

  SECTION("empty selector returns nullopt") {
    CHECK_FALSE(conky::select_wayland_output(outputs, "").has_value());
  }

  SECTION("matches by wl_output name") {
    CHECK(conky::select_wayland_output(outputs, "DP-3") == 0);
    CHECK(conky::select_wayland_output(outputs, "DP-2") == 1);
    CHECK(conky::select_wayland_output(outputs, "DP-1") == 2);
  }

  SECTION("matches by wl_output description") {
    CHECK(conky::select_wayland_output(outputs, "Dell U2415") == 1);
    CHECK(conky::select_wayland_output(outputs, "ASUS PA278QV") == 2);
  }

  SECTION("matches by 0-based ordinal index") {
    CHECK(conky::select_wayland_output(outputs, "0") == 0);
    CHECK(conky::select_wayland_output(outputs, "2") == 2);
    CHECK_FALSE(conky::select_wayland_output(outputs, "3").has_value());
    CHECK_FALSE(conky::select_wayland_output(outputs, "-1").has_value());
    CHECK_FALSE(
        conky::select_wayland_output(outputs, "99999999999999999999999999999")
            .has_value());
  }

  SECTION("exact name match takes precedence over ordinal index") {
    std::vector<fake_output> numeric_names = {
        {"1", "First"},
        {"0", "Second"},
    };
    CHECK(conky::select_wayland_output(numeric_names, "1") == 0);
    CHECK(conky::select_wayland_output(numeric_names, "0") == 1);
  }

  SECTION("unknown selector returns nullopt") {
    CHECK_FALSE(conky::select_wayland_output(outputs, "HDMI-A-1").has_value());
  }

  SECTION("empty output list and null pointer entries are handled safely") {
    std::vector<fake_output> empty;
    CHECK_FALSE(conky::select_wayland_output(empty, "DP-2").has_value());
    CHECK_FALSE(conky::select_wayland_output(empty, "0").has_value());

    std::vector<std::unique_ptr<fake_output>> sparse;
    sparse.push_back(nullptr);
    sparse.push_back(
        std::make_unique<fake_output>(fake_output{"DP-2", "Dell U2415"}));
    CHECK(conky::select_wayland_output(sparse, "DP-2") == 1);
    CHECK_FALSE(conky::select_wayland_output(sparse, "0").has_value());
  }
}
#endif
