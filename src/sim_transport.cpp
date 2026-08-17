// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/sim_transport.hpp"

#include <utility>

namespace openxhc {
namespace {
Error closed_error() { return {ErrorCode::Disconnected, "transport closed"}; }
}  // namespace

SimTransport::SimTransport(IClock& clock) : clock_(clock) {}

Result<DeviceIdentity> SimTransport::identify() {
  if (closed_) {
    return closed_error();
  }
  return DeviceIdentity{0x10ce, 0xeb73, 0, "XHC MACH3 CARD", 0x0100};
}

Result<RawReport> SimTransport::read(std::chrono::milliseconds timeout) {
  static_cast<void>(timeout);
  if (closed_) {
    return closed_error();
  }
  if (reads_.empty()) {
    return Error{ErrorCode::Timeout, "script exhausted"};
  }
  auto result = std::move(reads_.front());
  reads_.pop_front();
  return result;
}

Status SimTransport::write(const RawReport& report) {
  if (closed_) {
    return Status::fail(ErrorCode::Disconnected, "transport closed");
  }
  writes_.push_back(report);
  return Status::ok();
}

void SimTransport::close() noexcept { closed_ = true; }

void SimTransport::enqueue_read(RawReport report) { reads_.emplace_back(std::move(report)); }

void SimTransport::enqueue_error(Error error) { reads_.emplace_back(std::move(error)); }

std::span<const RawReport> SimTransport::writes() const noexcept { return writes_; }
}  // namespace openxhc
