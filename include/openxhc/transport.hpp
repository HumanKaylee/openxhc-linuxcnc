// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "openxhc/device_identity.hpp"
#include "openxhc/raw_report.hpp"

#include <chrono>

namespace openxhc {
class IClock {
 public:
  virtual ~IClock() = default;
  virtual std::chrono::nanoseconds now() const noexcept = 0;
};

class ITransport {
 public:
  virtual ~ITransport() = default;
  virtual Result<DeviceIdentity> identify() = 0;
  virtual Result<RawReport> read(std::chrono::milliseconds timeout) = 0;
  virtual Status write(const RawReport& report) = 0;
  virtual void close() noexcept = 0;
};

class ManualClock final : public IClock {
 public:
  std::chrono::nanoseconds now() const noexcept override { return now_; }
  void advance(std::chrono::nanoseconds amount) noexcept { now_ += amount; }

 private:
  std::chrono::nanoseconds now_{};
};
}  // namespace openxhc
