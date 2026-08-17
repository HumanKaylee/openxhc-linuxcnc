// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "openxhc/transport.hpp"

#include <deque>
#include <span>
#include <vector>

namespace openxhc {
// Test-only scripted transport; dynamic storage is intentionally excluded from future motion paths.
class SimTransport final : public ITransport {
 public:
  explicit SimTransport(IClock& clock);

  Result<DeviceIdentity> identify() override;
  Result<RawReport> read(std::chrono::milliseconds timeout) override;
  Status write(const RawReport& report) override;
  void close() noexcept override;

  void enqueue_read(RawReport report);
  void enqueue_error(Error error);
  std::span<const RawReport> writes() const noexcept;

 private:
  [[maybe_unused]] IClock& clock_;
  std::deque<Result<RawReport>> reads_;
  std::vector<RawReport> writes_;
  bool closed_{};
};
}  // namespace openxhc
