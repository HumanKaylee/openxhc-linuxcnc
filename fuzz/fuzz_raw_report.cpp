// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/raw_report.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  (void)openxhc::parse_raw_report(std::span<const std::uint8_t>(data, size));
  return 0;
}
