// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/raw_report.hpp"
#include <algorithm>

namespace openxhc {
Result<RawReport> parse_raw_report(std::span<const std::uint8_t> input) {
  if (input.empty() || input.size() > RawReport{}.bytes.size()) {
    return Error{ErrorCode::InvalidReport, "HID report length must be 1 through 64 bytes"};
  }
  RawReport report{};
  std::copy(input.begin(), input.end(), report.bytes.begin());
  report.size = input.size();
  return report;
}
}
