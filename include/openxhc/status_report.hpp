// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "openxhc/error.hpp"
#include "openxhc/raw_report.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace openxhc {
// The device-initiated record observed on the interrupt IN endpoint.
//
// Only what the captures actually established is encoded here: the record length and
// its leading byte. No field division, offset meaning, axis, input, or status
// semantics is claimed - those require correlation against deliberate physical
// changes, which has not been done.
inline constexpr std::size_t kStatusRecordBytes = 38U;
inline constexpr std::uint8_t kStatusRecordLeadingByte = 0x04U;

struct StatusRecord {
  std::array<std::uint8_t, kStatusRecordBytes> bytes{};
};

Result<StatusRecord> parse_status_record(const RawReport& report);

// Accumulates which byte offsets ever differ between records. This is a measurement
// aid for future correlation work, not an interpretation: it reports where the record
// is dynamic, never what the dynamic bytes mean.
class FieldActivity {
 public:
  void observe(const StatusRecord& record) noexcept;
  bool changed(std::size_t offset) const noexcept;
  std::size_t changed_count() const noexcept;
  std::uint64_t observations() const noexcept { return observations_; }

 private:
  std::array<std::uint8_t, kStatusRecordBytes> first_{};
  std::array<bool, kStatusRecordBytes> changed_{};
  std::uint64_t observations_{};
};
}  // namespace openxhc
