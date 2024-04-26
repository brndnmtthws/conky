// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2024 The Eigen Authors
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifdef _WIN32
#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <ctime>
#endif

extern "C" {
double dsecnd_();
}

// Elapsed CPU Time in seconds.
double dsecnd_() {
#ifdef _WIN32
  // For MSVC, use `GetProcessTimes` for proper CPU time - MSVC uses
  // a non-standard `std::clock` implementation (see
  // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/clock?view=msvc-170).
  // GetProcessTimes() uses 100-nanosecond time units.
  FILETIME creation_time, exit_time, kernel_time, user_time;
  GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time);
  ULARGE_INTEGER user;
  user.HighPart = user_time.dwHighDateTime;
  user.LowPart = user_time.dwLowDateTime;
  uint64_t time_100ns = user.QuadPart;
  return static_cast<double>(time_100ns) / 10000000.0;
#else
  return static_cast<double>(std::clock()) / static_cast<double>(CLOCKS_PER_SEC);
#endif
}
