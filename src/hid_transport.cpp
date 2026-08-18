// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/hid_transport.hpp"

#include "openxhc/hid_probe.hpp"

#include <memory>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#ifdef OPENXHC_WITH_HIDAPI
#include <hidapi/hidapi.h>
#endif

namespace openxhc {
HidTransport::HidTransport(std::unique_ptr<IHidBackend> backend,
                           DeviceIdentity identity) noexcept
    : backend_(std::move(backend)), identity_(std::move(identity)) {}

Result<DeviceIdentity> HidTransport::identify() {
  if (closed_ || backend_ == nullptr) {
    return Error{ErrorCode::Disconnected, "HID transport is closed"};
  }
  return identity_;
}

Result<RawReport> HidTransport::read(std::chrono::milliseconds timeout) {
  if (closed_ || backend_ == nullptr) {
    return Error{ErrorCode::Disconnected, "HID transport is closed"};
  }

  RawReport report{};
  const int milliseconds = timeout.count() > 0 ? static_cast<int>(timeout.count()) : 0;
  const int result = backend_->read(report.bytes.data(), report.bytes.size(), milliseconds);
  if (result < 0) {
    return Error{ErrorCode::Disconnected, "HID read failed"};
  }
  if (result == 0) {
    return Error{ErrorCode::Timeout, "HID read timed out"};
  }
  // Reject rather than truncate: a record longer than the fixed capacity means the
  // device or backend disagrees with the profile, and silently dropping the tail
  // would corrupt every downstream field offset.
  if (static_cast<std::size_t>(result) > report.bytes.size()) {
    return Error{ErrorCode::InvalidReport, "HID read exceeded the fixed report capacity"};
  }
  report.size = static_cast<std::size_t>(result);
  return report;
}

Status HidTransport::write(const RawReport& report) {
  // Deliberate: no output report for this controller has been captured, decoded, or
  // bench verified. The transport must not be able to emit one, closed or not.
  static_cast<void>(report);
  return Status::fail(ErrorCode::WriteDisabled,
                      "Writing to the controller is disabled at the current evidence gate");
}

bool is_recoverable_read_error(ErrorCode code) noexcept {
  return code == ErrorCode::Disconnected;
}

void HidTransport::close() noexcept {
  if (closed_) {
    return;
  }
  closed_ = true;
  if (backend_ != nullptr) {
    backend_->close();
  }
}

#ifdef OPENXHC_WITH_HIDAPI
namespace {
constexpr std::uint16_t kXhcVendorId = 0x10ce;
constexpr std::uint16_t kXhcProductId = 0xeb73;

class HidSession final {
 public:
  HidSession() = default;
  HidSession(const HidSession&) = delete;
  HidSession& operator=(const HidSession&) = delete;
  ~HidSession() { static_cast<void>(hid_exit()); }
};

class HidapiBackend final : public IHidBackend {
 public:
  explicit HidapiBackend(hid_device* handle) noexcept : handle_(handle) {}
  HidapiBackend(const HidapiBackend&) = delete;
  HidapiBackend& operator=(const HidapiBackend&) = delete;
  ~HidapiBackend() override { close(); }

  int read(std::uint8_t* buffer, std::size_t capacity, int timeout_milliseconds) override {
    if (handle_ == nullptr) {
      return -1;
    }
    return hid_read_timeout(handle_, buffer, capacity, timeout_milliseconds);
  }

  void close() noexcept override {
    if (handle_ != nullptr) {
      hid_close(handle_);
      handle_ = nullptr;
    }
  }

 private:
  hid_device* handle_;
};
}  // namespace

Result<std::unique_ptr<HidTransport>> open_supported_hid_transport(int interface_number) {
  // Deliberately does NOT reuse enumerate_supported_hid_with_paths(). That helper opens
  // every matching interface and issues manufacturer/product/serial descriptor queries.
  // This controller resets itself roughly every 2.8 s while on a Linux host, and those
  // extra control transfers do not reliably complete inside that window - measured as
  // twelve consecutive open failures against a plain read that succeeded.
  //
  // hid_enumerate already reports the product string and release number, so the identity
  // can be built without opening anything. Exactly one open is performed, for reading.
  if (hid_init() != 0) {
    return Error{ErrorCode::Disconnected, "Unable to initialize HIDAPI"};
  }
  const HidSession session;

  using EnumerationPointer = std::unique_ptr<hid_device_info, decltype(&hid_free_enumeration)>;
  EnumerationPointer records(hid_enumerate(kXhcVendorId, kXhcProductId), &hid_free_enumeration);

  for (const hid_device_info* record = records.get(); record != nullptr; record = record->next) {
    if (record->vendor_id != kXhcVendorId || record->product_id != kXhcProductId ||
        record->interface_number != interface_number || record->path == nullptr) {
      continue;
    }

    auto identity = normalize_hid_identity(
        record->vendor_id, record->product_id, record->interface_number,
        select_identity_string(record->product_string, record->manufacturer_string),
        record->release_number);
    if (std::holds_alternative<Error>(identity)) {
      return std::get<Error>(std::move(identity));
    }
    DeviceIdentity normalized = std::get<DeviceIdentity>(std::move(identity));
    if (!is_supported_device(normalized)) {
      continue;
    }

    hid_device* handle = hid_open_path(record->path);
    if (handle == nullptr) {
      return Error{ErrorCode::Disconnected, "Unable to open the HID interface for reading"};
    }
    return std::make_unique<HidTransport>(std::make_unique<HidapiBackend>(handle),
                                          std::move(normalized));
  }
  return Error{ErrorCode::UnsupportedDevice, "Requested interface is not present"};
}
#else
Result<std::unique_ptr<HidTransport>> open_supported_hid_transport(int interface_number) {
  static_cast<void>(interface_number);
  return Error{ErrorCode::UnsupportedDevice, "OpenXHC was built without HIDAPI"};
}
#endif
}  // namespace openxhc
