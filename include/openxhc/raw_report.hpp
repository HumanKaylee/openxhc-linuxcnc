// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "openxhc/error.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace openxhc {
struct RawReport {
  std::array<std::uint8_t, 64> bytes{};
  std::size_t size{};
};
Result<RawReport> parse_raw_report(std::span<const std::uint8_t> input);
}
