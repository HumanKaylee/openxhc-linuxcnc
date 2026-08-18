// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/status_report.hpp"

namespace openxhc {
Result<StatusRecord> parse_status_record(const RawReport& report) {
  if (report.size != kStatusRecordBytes) {
    return Error{ErrorCode::InvalidReport,
                 "Status record length does not match the observed device profile"};
  }
  if (report.bytes[0] != kStatusRecordLeadingByte) {
    return Error{ErrorCode::InvalidReport,
                 "Status record leading byte does not match the observed device profile"};
  }
  StatusRecord record{};
  for (std::size_t index = 0U; index < kStatusRecordBytes; ++index) {
    record.bytes[index] = report.bytes[index];
  }
  return record;
}

void FieldActivity::observe(const StatusRecord& record) noexcept {
  if (observations_ == 0U) {
    first_ = record.bytes;
    ++observations_;
    return;
  }
  for (std::size_t index = 0U; index < kStatusRecordBytes; ++index) {
    if (record.bytes[index] != first_[index]) {
      changed_[index] = true;
    }
  }
  ++observations_;
}

bool FieldActivity::changed(std::size_t offset) const noexcept {
  return offset < kStatusRecordBytes && changed_[offset];
}

std::size_t FieldActivity::changed_count() const noexcept {
  std::size_t total = 0U;
  for (const bool flag : changed_) {
    if (flag) {
      ++total;
    }
  }
  return total;
}
}  // namespace openxhc
