/*
 * Conky, a system monitor, based on torsmo
 *
 * This file is licensed under GPLv3. See COPYING for details.
 */

#include "config.h"

#include <future>
#include <string>

#include "catch2/catch.hpp"
#include "data/exec.h"

namespace {
class direct_exec_command : public exec_cb {
 public:
  explicit direct_exec_command(const std::string &command)
      : exec_cb(1, true, command) {}

  std::string run() {
    work();
    return get_result_copy();
  }
};
}  // namespace

TEST_CASE("exec commands retain their complete payload", "[exec][command]") {
  const size_t command_length = GENERATE(254, 255, 256, 257, 1024, 4096);
  const std::string prefix = "printf %s ";
  const std::string payload(command_length - prefix.size(), 'x');
  direct_exec_command command(prefix + payload);

  REQUIRE(command.run() == payload);
}

TEST_CASE("exec command outer quotes preserve shell quoting",
          "[exec][command]") {
  const std::string payload = "a string with spaces";
  const std::string shell_command = "printf '%s' '" + payload + "'";
  const std::string wrapper =
      GENERATE(std::string{}, std::string{"'"}, std::string{"\""});
  direct_exec_command command(wrapper + shell_command + wrapper);

  REQUIRE(command.run() == payload);
}

TEST_CASE("long quoted exec commands retain their complete payload",
          "[exec][command]") {
  const std::string payload(1024, 'q');
  const std::string wrapper = GENERATE(std::string{"'"}, std::string{"\""});
  direct_exec_command command(wrapper + "printf %s " + payload + wrapper);

  REQUIRE(command.run() == payload);
}

TEST_CASE("concurrent exec commands own independent command storage",
          "[exec][command]") {
  auto run = [](char value) {
    direct_exec_command command("printf %s " + std::string(1024, value));
    return command.run();
  };
  auto first = std::async(std::launch::async, run, 'a');
  auto second = std::async(std::launch::async, run, 'b');

  REQUIRE(first.get() == std::string(1024, 'a'));
  REQUIRE(second.get() == std::string(1024, 'b'));
}
