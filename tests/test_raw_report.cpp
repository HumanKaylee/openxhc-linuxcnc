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

  std::array<std::uint8_t, 64> maximum{};
  maximum.front() = 0x16;
  maximum.back() = 0xe9;
  auto maximum_parsed = openxhc::parse_raw_report(maximum);
  CHECK(std::holds_alternative<openxhc::RawReport>(maximum_parsed));
  const auto& maximum_report = std::get<openxhc::RawReport>(maximum_parsed);
  CHECK(maximum_report.size == 64);
  CHECK(maximum_report.bytes.front() == 0x16);
  CHECK(maximum_report.bytes.back() == 0xe9);

  const std::array<std::uint8_t, 0> empty{};
  auto empty_parsed = openxhc::parse_raw_report(empty);
  CHECK(std::holds_alternative<openxhc::Error>(empty_parsed));
  const auto& empty_error = std::get<openxhc::Error>(empty_parsed);
  CHECK(empty_error.code == openxhc::ErrorCode::InvalidReport);

  const std::array<std::uint8_t, 65> oversized{};
  auto oversized_parsed = openxhc::parse_raw_report(oversized);
  CHECK(std::holds_alternative<openxhc::Error>(oversized_parsed));
  const auto& oversized_error = std::get<openxhc::Error>(oversized_parsed);
  CHECK(oversized_error.code == openxhc::ErrorCode::InvalidReport);
  return 0;
}
