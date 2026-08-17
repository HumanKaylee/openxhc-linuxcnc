// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/raw_report.hpp"
#include "test_support.hpp"
#include <array>

int main() {
  const std::array<std::uint8_t, 4> bytes{0x04, 0x00, 0xff, 0x7e};
  auto parsed = openxhc::parse_raw_report(bytes);
  CHECK(std::holds_alternative<openxhc::RawReport>(parsed));
  const auto& report = std::get<openxhc::RawReport>(parsed);
  CHECK(report.size == 4);
  CHECK(report.bytes[1] == 0x00);
  CHECK(report.bytes[2] == 0xff);

  const std::array<std::uint8_t, 0> empty{};
  CHECK(std::holds_alternative<openxhc::Error>(openxhc::parse_raw_report(empty)));
  const std::array<std::uint8_t, 65> oversized{};
  CHECK(std::holds_alternative<openxhc::Error>(openxhc::parse_raw_report(oversized)));
  return 0;
}
