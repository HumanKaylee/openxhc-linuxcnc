// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/hid_probe.hpp"

#include "openxhc/hid_transport.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

#ifdef OPENXHC_WITH_HIDAPI
#include <hidapi/hidapi.h>
#endif

namespace openxhc {
Result<DeviceIdentity> normalize_hid_identity(std::uint16_t vendor_id,
                                              std::uint16_t product_id,
                                              int interface_number,
                                              std::wstring_view product_string,
                                              std::uint16_t release_number) {
  std::string normalized;
  normalized.reserve(product_string.size());
  for (const wchar_t character : product_string) {
    if (character < 0x20 || character > 0x7e) {
      return Error{ErrorCode::ParseError, "HID product string is not printable ASCII"};
    }
    normalized.push_back(static_cast<char>(character));
  }
  return DeviceIdentity{vendor_id, product_id, interface_number, std::move(normalized),
                        release_number};
}

#ifdef OPENXHC_WITH_HIDAPI
namespace {
constexpr std::uint16_t kXhcVendorId = 0x10ce;
constexpr std::uint16_t kXhcProductId = 0xeb73;
constexpr std::size_t kDescriptorCharacters = 256U;

class HidSession final {
 public:
  HidSession() = default;
  HidSession(const HidSession&) = delete;
  HidSession& operator=(const HidSession&) = delete;
  ~HidSession() { static_cast<void>(hid_exit()); }
};

bool candidate_interface(int interface_number) {
  return interface_number == 0 || interface_number == 1;
}
}  // namespace

Result<std::vector<HidDeviceIdentity>> enumerate_supported_hid_with_paths() {
  if (hid_init() != 0) {
    return Error{ErrorCode::Disconnected, "Unable to initialize HIDAPI"};
  }
  const HidSession session;

  using EnumerationPointer =
      std::unique_ptr<hid_device_info, decltype(&hid_free_enumeration)>;
  EnumerationPointer records(hid_enumerate(kXhcVendorId, kXhcProductId),
                             &hid_free_enumeration);

  std::vector<HidDeviceIdentity> devices;
  for (const hid_device_info* record = records.get(); record != nullptr; record = record->next) {
    if (record->vendor_id != kXhcVendorId || record->product_id != kXhcProductId ||
        !candidate_interface(record->interface_number)) {
      continue;
    }
    if (record->path == nullptr) {
      return Error{ErrorCode::ParseError, "HID enumeration record has no path"};
    }

    using DevicePointer = std::unique_ptr<hid_device, decltype(&hid_close)>;
    DevicePointer handle(hid_open_path(record->path), &hid_close);
    if (handle == nullptr) {
      return Error{ErrorCode::Disconnected,
                   "Unable to open HID interface for descriptor query"};
    }

    std::array<wchar_t, kDescriptorCharacters> manufacturer{};
    std::array<wchar_t, kDescriptorCharacters> product{};
    std::array<wchar_t, kDescriptorCharacters> serial{};
    if (hid_get_manufacturer_string(handle.get(), manufacturer.data(), manufacturer.size()) < 0 ||
        hid_get_product_string(handle.get(), product.data(), product.size()) < 0 ||
        hid_get_serial_number_string(handle.get(), serial.data(), serial.size()) < 0) {
      return Error{ErrorCode::Disconnected, "Unable to query HID descriptors"};
    }

    // This controller has no product string descriptor - its identity is carried in the
    // manufacturer descriptor - so a product-only rule rejects the real device. Ask for
    // whichever descriptor actually carries a name.
    auto identity = normalize_hid_identity(
        record->vendor_id, record->product_id, record->interface_number,
        select_identity_string(product.data(), manufacturer.data()), record->release_number);
    if (std::holds_alternative<Error>(identity)) {
      return std::get<Error>(std::move(identity));
    }
    DeviceIdentity normalized = std::get<DeviceIdentity>(std::move(identity));
    if (is_supported_device(normalized)) {
      devices.push_back(HidDeviceIdentity{std::move(normalized), record->path});
    }
  }

  if (devices.empty()) {
    return Error{ErrorCode::UnsupportedDevice, "No supported XHC HID interfaces found"};
  }
  std::sort(devices.begin(), devices.end(),
            [](const HidDeviceIdentity& left, const HidDeviceIdentity& right) {
              if (left.identity.interface_number != right.identity.interface_number) {
                return left.identity.interface_number < right.identity.interface_number;
              }
              return left.path < right.path;
            });
  return devices;
}

Result<std::vector<DeviceIdentity>> enumerate_supported_hid() {
  auto detailed = enumerate_supported_hid_with_paths();
  if (std::holds_alternative<Error>(detailed)) {
    return std::get<Error>(std::move(detailed));
  }

  auto& detailed_devices = std::get<std::vector<HidDeviceIdentity>>(detailed);
  std::vector<DeviceIdentity> identities;
  identities.reserve(detailed_devices.size());
  for (auto& device : detailed_devices) {
    identities.push_back(std::move(device.identity));
  }
  return identities;
}
#else
Result<std::vector<DeviceIdentity>> enumerate_supported_hid() {
  return Error{ErrorCode::UnsupportedDevice, "OpenXHC was built without HIDAPI"};
}

Result<std::vector<HidDeviceIdentity>> enumerate_supported_hid_with_paths() {
  return Error{ErrorCode::UnsupportedDevice, "OpenXHC was built without HIDAPI"};
}
#endif
}  // namespace openxhc
