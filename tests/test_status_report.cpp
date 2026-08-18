// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/status_report.hpp"
#include "test_support.hpp"

#include <variant>

namespace {
using openxhc::Error;
using openxhc::ErrorCode;
using openxhc::FieldActivity;
using openxhc::RawReport;
using openxhc::StatusRecord;

// Synthetic record of the observed shape. Not capture data.
RawReport synthetic(std::uint8_t leading, std::size_t size) {
  RawReport report{};
  report.size = size;
  if (size > 0U) {
    report.bytes[0] = leading;
  }
  for (std::size_t index = 1U; index < size; ++index) {
    report.bytes[index] = 0x00U;
  }
  return report;
}

template <class T>
bool has_error(const openxhc::Result<T>& result, ErrorCode code) {
  return std::holds_alternative<Error>(result) && std::get<Error>(result).code == code;
}
}  // namespace

int main() {
  // A record of the observed length and leading byte is accepted.
  auto good = openxhc::parse_status_record(synthetic(0x04U, 38U));
  CHECK(std::holds_alternative<StatusRecord>(good));
  CHECK(std::get<StatusRecord>(good).bytes[0] == 0x04U);

  // NEGATIVE: wrong length, in both directions, and empty.
  CHECK(has_error(openxhc::parse_status_record(synthetic(0x04U, 37U)), ErrorCode::InvalidReport));
  CHECK(has_error(openxhc::parse_status_record(synthetic(0x04U, 39U)), ErrorCode::InvalidReport));
  CHECK(has_error(openxhc::parse_status_record(synthetic(0x04U, 0U)), ErrorCode::InvalidReport));
  // NEGATIVE: right length, wrong leading byte.
  CHECK(has_error(openxhc::parse_status_record(synthetic(0x05U, 38U)), ErrorCode::InvalidReport));

  // Activity tracking reports where the record is dynamic, and nothing more.
  FieldActivity activity;
  CHECK(activity.observations() == 0U);
  CHECK(activity.changed_count() == 0U);

  auto base = std::get<StatusRecord>(openxhc::parse_status_record(synthetic(0x04U, 38U)));
  activity.observe(base);
  CHECK(activity.observations() == 1U);
  // A single observation cannot establish change.
  CHECK(activity.changed_count() == 0U);

  // Identical records add observations but no activity.
  activity.observe(base);
  CHECK(activity.observations() == 2U);
  CHECK(activity.changed_count() == 0U);

  StatusRecord moved = base;
  moved.bytes[9] = 0x7fU;
  moved.bytes[33] = 0x01U;
  activity.observe(moved);
  CHECK(activity.observations() == 3U);
  CHECK(activity.changed_count() == 2U);
  CHECK(activity.changed(9U));
  CHECK(activity.changed(33U));
  // NEGATIVE: untouched offsets must not be reported as active.
  CHECK(!activity.changed(0U));
  CHECK(!activity.changed(37U));
  // NEGATIVE: out-of-range offsets are never reported active.
  CHECK(!activity.changed(38U));
  CHECK(!activity.changed(1000U));

  // Activity is sticky: a later record matching the first does not clear it.
  activity.observe(base);
  CHECK(activity.changed(9U));
  CHECK(activity.changed_count() == 2U);

  return 0;
}
