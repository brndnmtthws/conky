/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "catch2/catch.hpp"

#ifdef __linux__
#include "data/os/linux.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace {
struct hwmon_tree {
  std::filesystem::path root;

  hwmon_tree() {
    char pattern[] = "/tmp/conky-hwmon-XXXXXX";
    const auto *directory = mkdtemp(pattern);
    REQUIRE(directory != nullptr);
    root = directory;
    std::filesystem::create_directories(root / "class/hwmon");
  }
  ~hwmon_tree() {
    std::error_code error;
    std::filesystem::remove_all(root, error);
  }

  void add(const std::string &index, const std::string &disk,
           const std::string &module = "drivetemp", bool legacy = false) {
    const auto device = root / "devices" / disk;
    const auto hwmon = device / "hwmon" / index;
    std::filesystem::create_directories(device / "block" / disk);
    std::filesystem::create_directories(hwmon);
    std::filesystem::create_directory_symlink(device, hwmon / "device");
    std::filesystem::create_directory_symlink(hwmon,
                                              root / "class/hwmon" / index);
    std::ofstream((legacy ? device : hwmon) / "name") << module << '\n';
    std::ofstream(hwmon / "temp1_input") << (disk == "sda" ? 31000 : 42000);
  }

  std::string resolve(const char *selector) {
    char result[512] = {};
    get_hwmon_dev_path((root / "class/hwmon").string().append("/").c_str(),
                       selector, result);
    return result;
  }

  int temperature(const char *selector) {
    const auto path = root / "class/hwmon" / resolve(selector);
    std::ifstream input(path.parent_path() / "temp1_input");
    int value = 0;
    input >> value;
    REQUIRE_FALSE(input.fail());
    return value;
  }
};
}  // namespace

TEST_CASE("hwmon block selectors survive hwmon renumbering", "[linux][hwmon]") {
  hwmon_tree tree;
  tree.add("hwmon2", "sda");
  tree.add("hwmon3", "sdb");
  CHECK(tree.resolve("drivetemp@sda") == "hwmon2/device");
  CHECK(tree.resolve("drivetemp@sdb") == "hwmon3/device");
  CHECK(tree.temperature("drivetemp@sda") == 31000);
  CHECK(tree.temperature("drivetemp@sdb") == 42000);

  const auto devices = tree.root / "class/hwmon";
  std::filesystem::rename(devices / "hwmon2", devices / "hwmon9");
  std::filesystem::rename(devices / "hwmon3", devices / "hwmon0");
  CHECK(tree.resolve("drivetemp@sda") == "hwmon9/device");
  CHECK(tree.resolve("drivetemp@sdb") == "hwmon0/device");
  CHECK(tree.temperature("drivetemp@sda") == 31000);
  CHECK(tree.temperature("drivetemp@sdb") == 42000);
}

TEST_CASE("hwmon retains legacy selectors and name locations",
          "[linux][hwmon]") {
  hwmon_tree tree;
  tree.add("hwmon2", "sda", "drivetemp", true);
  tree.add("hwmon3", "sdb");
  CHECK(tree.resolve("2") == "hwmon2/device");
  CHECK(tree.resolve("drivetemp") == "hwmon2/device");
  CHECK(tree.resolve("drivetemp@sda") == "hwmon2/device");
  CHECK(tree.resolve("drivetemp@sdb") == "hwmon3/device");
}

TEST_CASE("hwmon qualified selectors never fall back to another drive",
          "[linux][hwmon]") {
  hwmon_tree tree;
  tree.add("hwmon2", "sda");
  for (const auto *selector :
       {"drivetemp@sdb", "drive@sda", "@sda", "drivetemp@", "drivetemp@..",
        "drivetemp@sda/../sdb", "drivetemp@sda@other", "drivetemp@sda1"}) {
    INFO(selector);
    CHECK(tree.resolve(selector).empty());
  }
  std::filesystem::remove_all(tree.root / "devices/sda/block/sda");
  CHECK(tree.resolve("drivetemp@sda").empty());
  std::ofstream(tree.root / "devices/sda/block/sda") << "not a directory";
  CHECK(tree.resolve("drivetemp@sda").empty());
  std::filesystem::remove(tree.root / "devices/sda/block/sda");
  std::filesystem::create_directory(tree.root / "devices/sda/block/sda");
  std::filesystem::create_directory_symlink(
      tree.root / "devices/sda/hwmon/hwmon2", tree.root / "class/hwmon/hwmon4");
  CHECK(tree.resolve("drivetemp@sda").empty());
  std::filesystem::remove_all(tree.root / "class/hwmon");
  CHECK(tree.resolve("drivetemp@sda").empty());
}
#endif
