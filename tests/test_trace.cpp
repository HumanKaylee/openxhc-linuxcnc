// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/trace.hpp"
#include "test_support.hpp"
#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {
using openxhc::Direction;
using openxhc::Error;
using openxhc::ErrorCode;
using openxhc::Result;
using openxhc::TraceRecord;

template <class T>
bool has_parse_error(const Result<T>& result) {
  return std::holds_alternative<Error>(result) &&
         std::get<Error>(result).code == ErrorCode::ParseError;
}

std::vector<std::string> read_lines(std::string_view filename) {
  std::ifstream input{std::string(filename)};
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(input, line)) {
    lines.push_back(line);
  }
  return lines;
}

std::string fixture_path(std::string_view filename) {
  return std::string(OPENXHC_TEST_FIXTURE_DIR) + "/" + std::string(filename);
}

bool same_record(const TraceRecord& left, const TraceRecord& right) {
  if (left.timestamp_ns != right.timestamp_ns || left.direction != right.direction ||
      left.report.size != right.report.size) {
    return false;
  }
  for (std::size_t index = 0; index < left.report.size; ++index) {
    if (left.report.bytes[index] != right.report.bytes[index]) {
      return false;
    }
  }
  return true;
}
} // namespace

int main() {
  const auto native_lines = read_lines(fixture_path("baseline.xhctrace"));
  CHECK(native_lines.size() == 4U);
  CHECK(native_lines[0] == "OPENXHC_TRACE_V1");

  std::array<TraceRecord, 3> records{};
  for (std::size_t index = 0; index < records.size(); ++index) {
    auto parsed = openxhc::parse_trace_line(native_lines[index + 1U]);
    CHECK(std::holds_alternative<TraceRecord>(parsed));
    records[index] = std::get<TraceRecord>(parsed);
  }
  CHECK(records[0].timestamp_ns == 1000U);
  CHECK(records[0].direction == Direction::DeviceToHost);
  CHECK(records[0].report.size == 4U);
  CHECK(records[0].report.bytes[0] == 0x04U);
  CHECK(records[1].direction == Direction::HostToDevice);
  CHECK(records[2].report.bytes[1] == 0xffU);

  for (std::size_t index = 0; index < records.size(); ++index) {
    std::ostringstream serialized;
    serialized << std::hex << std::showbase;
    const auto caller_flags = serialized.flags();
    CHECK(openxhc::write_trace_record(serialized, records[index]).success);
    CHECK(serialized.flags() == caller_flags);
    const auto expected_line = native_lines[index + 1U] + "\n";
    CHECK(serialized.str() == expected_line);
    auto round_trip = openxhc::parse_trace_line(
        serialized.str().substr(0U, serialized.str().size() - 1U));
    CHECK(std::holds_alternative<TraceRecord>(round_trip));
    CHECK(same_record(records[index], std::get<TraceRecord>(round_trip)));
  }

  const auto tshark_lines = read_lines(fixture_path("tshark-baseline.tsv"));
  CHECK(tshark_lines.size() == 2U);
  auto tshark_in = openxhc::parse_tshark_line(tshark_lines[0]);
  auto tshark_out = openxhc::parse_tshark_line(tshark_lines[1]);
  CHECK(std::holds_alternative<TraceRecord>(tshark_in));
  CHECK(std::holds_alternative<TraceRecord>(tshark_out));
  CHECK(std::get<TraceRecord>(tshark_in).timestamp_ns == 1000U);
  CHECK(std::get<TraceRecord>(tshark_in).direction == Direction::DeviceToHost);
  CHECK(std::get<TraceRecord>(tshark_out).timestamp_ns == 2000U);
  CHECK(std::get<TraceRecord>(tshark_out).direction == Direction::HostToDevice);
  CHECK(std::get<TraceRecord>(tshark_out).report.bytes[0] == 0x0bU);
  auto rounded_tshark = openxhc::parse_tshark_line("0.0000000005\t0x81\t04");
  CHECK(std::holds_alternative<TraceRecord>(rounded_tshark));
  CHECK(std::get<TraceRecord>(rounded_tshark).timestamp_ns == 1U);

  const auto summary = openxhc::summarize_trace(records);
  CHECK(summary.records == 3U);
  CHECK(summary.device_to_host == 2U);
  CHECK(summary.host_to_device == 1U);
  CHECK(summary.report_ids[0x04U] == 2U);
  CHECK(summary.report_ids[0x0bU] == 1U);
  CHECK(summary.lengths[4U] == 3U);

  CHECK(has_parse_error(openxhc::parse_trace_line("\tIN\t04000102")));
  CHECK(has_parse_error(openxhc::parse_tshark_line("\t0x81\t04:00:01:02")));
  CHECK(has_parse_error(openxhc::parse_tshark_line("0.1\t0x83\t04")));
  CHECK(has_parse_error(openxhc::parse_trace_line("1000\tIN\t040")));
  CHECK(has_parse_error(openxhc::parse_tshark_line("0.1\t0x81\t04:0g")));
  CHECK(has_parse_error(openxhc::parse_tshark_line("0.1\t0x81\t04:")));
  CHECK(has_parse_error(openxhc::parse_trace_line("1000\tIN")));
  CHECK(has_parse_error(openxhc::parse_trace_line("1000\tSIDE\t04")));
  CHECK(has_parse_error(openxhc::parse_trace_line("1000\tIN\t04\textra")));
  std::string oversized = "1000\tIN\t";
  for (std::size_t index = 0; index < 65U; ++index) {
    oversized += "00";
  }
  CHECK(has_parse_error(openxhc::parse_trace_line(oversized)));
  std::string oversized_tshark = "0.1\t0x81\t";
  for (std::size_t index = 0; index < 65U; ++index) {
    if (index != 0U) {
      oversized_tshark += ':';
    }
    oversized_tshark += "00";
  }
  CHECK(has_parse_error(openxhc::parse_tshark_line(oversized_tshark)));
  CHECK(has_parse_error(openxhc::parse_tshark_line("18446744073709551616\t0x81\t04")));
  return 0;
}
