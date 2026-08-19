// SPDX-License-Identifier: GPL-2.0-or-later
//
// The status record is the only structure this driver parses from live device data, so it
// is the one decoder an untrusted or malfunctioning controller can reach directly. The raw
// report fuzzer covers the envelope; this covers what is built on top of it, including the
// activity accumulator that walks the record.
#include "openxhc/raw_report.hpp"
#include "openxhc/status_report.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  // Drive the same path the transport does: envelope first, then the status decoder.
  auto envelope = openxhc::parse_raw_report(std::span<const std::uint8_t>(data, size));
  if (std::holds_alternative<openxhc::Error>(envelope)) {
    return 0;
  }

  const auto& report = std::get<openxhc::RawReport>(envelope);
  auto record = openxhc::parse_status_record(report);
  if (std::holds_alternative<openxhc::Error>(record)) {
    return 0;
  }

  // Accepted records feed the accumulator, which indexes the record by offset. Run it
  // twice so the "first observation" and "compare against first" branches both execute.
  openxhc::FieldActivity activity;
  const auto& accepted = std::get<openxhc::StatusRecord>(record);
  activity.observe(accepted);
  activity.observe(accepted);
  for (std::size_t offset = 0U; offset <= openxhc::kStatusRecordBytes + 1U; ++offset) {
    (void)activity.changed(offset);
  }
  (void)activity.changed_count();
  return 0;
}
