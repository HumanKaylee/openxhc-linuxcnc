// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "openxhc/device_identity.hpp"
#include "openxhc/error.hpp"
#include "openxhc/raw_report.hpp"
#include "openxhc/transport.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace openxhc {
// Minimal seam over the HID backend so the transport's policy - length checking,
// timeout handling, and the refusal to write - is testable without hardware.
//
// There is deliberately no write operation on this interface. A backend that
// cannot express a write cannot be talked into performing one.
class IHidBackend {
 public:
  virtual ~IHidBackend() = default;
  // Returns the number of bytes read, 0 on timeout, or a negative value on error.
  virtual int read(std::uint8_t* buffer, std::size_t capacity, int timeout_milliseconds) = 0;
  virtual void close() noexcept = 0;
};

// Read-only transport over a HID interrupt IN endpoint.
//
// `write` always fails with ErrorCode::WriteDisabled and performs no I/O. This is a
// safety property of the current evidence gate, not a placeholder: no output report
// for this controller has been captured, decoded, or bench verified, so the driver
// must not be able to emit one.
class HidTransport final : public ITransport {
 public:
  HidTransport(std::unique_ptr<IHidBackend> backend, DeviceIdentity identity) noexcept;

  Result<DeviceIdentity> identify() override;
  Result<RawReport> read(std::chrono::milliseconds timeout) override;
  Status write(const RawReport& report) override;
  void close() noexcept override;

  bool closed() const noexcept { return closed_; }

 private:
  std::unique_ptr<IHidBackend> backend_;
  DeviceIdentity identity_;
  bool closed_{false};
};

// Opens the exact supported controller for reading. Available only when built with
// HIDAPI; otherwise returns ErrorCode::UnsupportedDevice.
Result<std::unique_ptr<HidTransport>> open_supported_hid_transport(int interface_number);

// Whether a read failure warrants reopening the device rather than giving up.
//
// This controller resets itself roughly every 2.8 s while attached to a Linux host, so a
// disconnect mid-stream is the expected environment, not a fault. A timeout means keep
// reading on the same handle; a malformed record is a data problem and reopening would
// only hide it.
bool is_recoverable_read_error(ErrorCode code) noexcept;
}  // namespace openxhc
