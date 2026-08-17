// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "openxhc/error.hpp"
#include "openxhc/raw_report.hpp"
#include <array>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <string_view>

namespace openxhc {
enum class Direction { DeviceToHost, HostToDevice };

struct TraceRecord {
  std::uint64_t timestamp_ns;
  Direction direction;
  RawReport report;
};

struct TraceSummary {
  std::uint64_t records{};
  std::uint64_t device_to_host{};
  std::uint64_t host_to_device{};
  std::array<std::uint64_t, 256> report_ids{};
  std::array<std::uint64_t, 65> lengths{};
};

Result<TraceRecord> parse_trace_line(std::string_view line);
Result<TraceRecord> parse_tshark_line(std::string_view line);
Status write_trace_record(std::ostream& output, const TraceRecord& record);
TraceSummary summarize_trace(std::span<const TraceRecord> records) noexcept;
} // namespace openxhc
