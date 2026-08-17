// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <iostream>

#define CHECK(expression)                                                        \
  do {                                                                           \
    if (!(expression)) {                                                         \
      std::cerr << "CHECK failed: " #expression " at " << __FILE__ << ':'    \
                << __LINE__ << '\n';                                             \
      return 1;                                                                  \
    }                                                                            \
  } while (false)
