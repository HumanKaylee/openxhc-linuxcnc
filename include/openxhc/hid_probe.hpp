// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "openxhc/device_identity.hpp"
#include "openxhc/error.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace openxhc {
struct HidDeviceIdentity {
  DeviceIdentity identity;
  // Available only through the explicit local-diagnostics API; default discovery omits paths.
  std::string path;
};

Result<DeviceIdentity> normalize_hid_identity(std::uint16_t vendor_id,
                                              std::uint16_t product_id,
                                              int interface_number,
                                              std::wstring_view product_string,
                                              std::uint16_t release_number);

// Chooses which enumeration descriptor carries the device's identity string.
//
// This controller has NO product string descriptor. Its identity, "XHC MACH3 CARD", is
// carried in the MANUFACTURER descriptor, and hid_enumerate reports product_string as
// null. Opening the device and calling hid_get_product_string() hides that, because the
// hidraw backend synthesises that value from udev's HID_NAME - which is itself derived
// from the manufacturer string. Prefer the product descriptor where a device provides
// one, and fall back to the manufacturer descriptor, so identification works whether or
// not the device has been opened.
std::wstring_view select_identity_string(const wchar_t* product_string,
                                         const wchar_t* manufacturer_string) noexcept;

Result<std::vector<DeviceIdentity>> enumerate_supported_hid();
Result<std::vector<HidDeviceIdentity>> enumerate_supported_hid_with_paths();
}  // namespace openxhc
