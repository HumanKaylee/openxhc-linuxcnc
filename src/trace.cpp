// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/trace.hpp"
#include <limits>
#include <ostream>

namespace openxhc {
namespace {
constexpr std::uint64_t kNanosecondsPerSecond = 1000000000U;
constexpr std::string_view kHexDigits = "0123456789abcdef";

Error parse_error(const char* message) { return {ErrorCode::ParseError, message}; }

bool digit_value(char character, std::uint8_t& value) noexcept {
  if (character >= '0' && character <= '9') {
    value = static_cast<std::uint8_t>(character - '0');
    return true;
  }
  if (character >= 'a' && character <= 'f') {
    value = static_cast<std::uint8_t>(character - 'a' + 10);
    return true;
  }
  if (character >= 'A' && character <= 'F') {
    value = static_cast<std::uint8_t>(character - 'A' + 10);
    return true;
  }
  return false;
}

bool parse_uint64(std::string_view text, std::uint64_t& value) noexcept {
  if (text.empty()) {
    return false;
  }
  value = 0U;
  for (const char character : text) {
    if (character < '0' || character > '9') {
      return false;
    }
    const auto digit = static_cast<std::uint64_t>(character - '0');
    if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
      return false;
    }
    value = value * 10U + digit;
  }
  return true;
}

bool parse_decimal_seconds(std::string_view text, std::uint64_t& timestamp_ns) noexcept {
  const auto decimal = text.find('.');
  const auto seconds_text = text.substr(0U, decimal);
  std::uint64_t seconds{};
  if (!parse_uint64(seconds_text, seconds)) {
    return false;
  }

  std::uint64_t fractional_ns{};
  bool round_up = false;
  if (decimal != std::string_view::npos) {
    const auto fraction = text.substr(decimal + 1U);
    if (fraction.empty()) {
      return false;
    }
    for (std::size_t index = 0U; index < fraction.size(); ++index) {
      const char character = fraction[index];
      if (character < '0' || character > '9') {
        return false;
      }
      if (index < 9U) {
        fractional_ns = fractional_ns * 10U + static_cast<std::uint64_t>(character - '0');
      } else if (index == 9U && character >= '5') {
        round_up = true;
      }
    }
    for (std::size_t index = fraction.size(); index < 9U; ++index) {
      fractional_ns *= 10U;
    }
  }

  if (seconds > (std::numeric_limits<std::uint64_t>::max() - fractional_ns) /
                    kNanosecondsPerSecond) {
    return false;
  }
  timestamp_ns = seconds * kNanosecondsPerSecond + fractional_ns;
  if (round_up) {
    if (timestamp_ns == std::numeric_limits<std::uint64_t>::max()) {
      return false;
    }
    ++timestamp_ns;
  }
  return true;
}

bool split_three_fields(std::string_view line, std::array<std::string_view, 3>& fields) noexcept {
  const auto first = line.find('\t');
  if (first == std::string_view::npos) {
    return false;
  }
  const auto second = line.find('\t', first + 1U);
  if (second == std::string_view::npos || line.find('\t', second + 1U) != std::string_view::npos) {
    return false;
  }
  fields = {line.substr(0U, first), line.substr(first + 1U, second - first - 1U),
            line.substr(second + 1U)};
  return !fields[0].empty() && !fields[1].empty() && !fields[2].empty();
}

bool parse_native_payload(std::string_view text, RawReport& report) noexcept {
  if (text.empty() || text.size() % 2U != 0U || text.size() > report.bytes.size() * 2U) {
    return false;
  }
  report.size = text.size() / 2U;
  for (std::size_t index = 0U; index < report.size; ++index) {
    std::uint8_t high{};
    std::uint8_t low{};
    if (!digit_value(text[index * 2U], high) || !digit_value(text[index * 2U + 1U], low)) {
      return false;
    }
    report.bytes[index] = static_cast<std::uint8_t>((high << 4U) | low);
  }
  return true;
}

bool parse_tshark_payload(std::string_view text, RawReport& report) noexcept {
  if (text.empty()) {
    return false;
  }
  report.size = 0U;
  std::size_t offset{};
  while (offset < text.size()) {
    if (report.size == report.bytes.size() || offset + 2U > text.size()) {
      return false;
    }
    std::uint8_t high{};
    std::uint8_t low{};
    if (!digit_value(text[offset], high) || !digit_value(text[offset + 1U], low)) {
      return false;
    }
    report.bytes[report.size++] = static_cast<std::uint8_t>((high << 4U) | low);
    offset += 2U;
    if (offset == text.size()) {
      break;
    }
    if (text[offset] != ':') {
      return false;
    }
    ++offset;
    if (offset == text.size()) {
      return false;
    }
  }
  return report.size != 0U;
}

void write_uint64(std::ostream& output, std::uint64_t value) {
  char digits[20];
  std::size_t size{};
  do {
    digits[size++] = static_cast<char>('0' + (value % 10U));
    value /= 10U;
  } while (value != 0U);
  while (size > 0U) {
    --size;
    output.put(digits[size]);
  }
}
} // namespace

Result<TraceRecord> parse_trace_line(std::string_view line) {
  std::array<std::string_view, 3> fields{};
  if (!split_three_fields(line, fields)) {
    return parse_error("native trace record must contain three tab-separated fields");
  }
  std::uint64_t timestamp_ns{};
  if (!parse_uint64(fields[0], timestamp_ns)) {
    return parse_error("native trace timestamp must be an unsigned integer nanosecond count");
  }
  Direction direction{};
  if (fields[1] == "IN") {
    direction = Direction::DeviceToHost;
  } else if (fields[1] == "OUT") {
    direction = Direction::HostToDevice;
  } else {
    return parse_error("native trace direction must be IN or OUT");
  }
  RawReport report{};
  if (!parse_native_payload(fields[2], report)) {
    return parse_error("native trace report must contain one through 64 hexadecimal bytes");
  }
  return TraceRecord{timestamp_ns, direction, report};
}

Result<TraceRecord> parse_tshark_line(std::string_view line) {
  std::array<std::string_view, 3> fields{};
  if (!split_three_fields(line, fields)) {
    return parse_error("TShark record must contain three tab-separated fields");
  }
  std::uint64_t timestamp_ns{};
  if (!parse_decimal_seconds(fields[0], timestamp_ns)) {
    return parse_error("TShark timestamp must be a checked decimal seconds value");
  }
  Direction direction{};
  if (fields[1] == "0x81") {
    direction = Direction::DeviceToHost;
  } else if (fields[1] == "0x02") {
    direction = Direction::HostToDevice;
  } else {
    return parse_error("TShark endpoint is not supported by the initial device profile");
  }
  RawReport report{};
  if (!parse_tshark_payload(fields[2], report)) {
    return parse_error("TShark report must contain one through 64 colon-separated hexadecimal bytes");
  }
  return TraceRecord{timestamp_ns, direction, report};
}

Status write_trace_record(std::ostream& output, const TraceRecord& record) {
  if (record.report.size == 0U || record.report.size > record.report.bytes.size()) {
    return Status::fail(ErrorCode::ParseError, "trace record report length must be 1 through 64 bytes");
  }
  write_uint64(output, record.timestamp_ns);
  output.write("\t", 1);
  if (record.direction == Direction::DeviceToHost) {
    output.write("IN", 2);
  } else {
    output.write("OUT", 3);
  }
  output.write("\t", 1);
  for (std::size_t index = 0U; index < record.report.size; ++index) {
    const auto byte = record.report.bytes[index];
    output.put(kHexDigits[byte >> 4U]);
    output.put(kHexDigits[byte & 0x0fU]);
  }
  output.put('\n');
  if (!output) {
    return Status::fail(ErrorCode::ParseError, "failed to write trace record");
  }
  return Status::ok();
}

TraceSummary summarize_trace(std::span<const TraceRecord> records) noexcept {
  TraceSummary summary{};
  summary.records = records.size();
  for (const TraceRecord& record : records) {
    if (record.direction == Direction::DeviceToHost) {
      ++summary.device_to_host;
    } else {
      ++summary.host_to_device;
    }
    if (record.report.size > 0U && record.report.size <= record.report.bytes.size()) {
      ++summary.report_ids[record.report.bytes[0]];
      ++summary.lengths[record.report.size];
    }
  }
  return summary;
}
} // namespace openxhc
