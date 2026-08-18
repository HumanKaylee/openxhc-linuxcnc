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

Result<std::vector<DeviceIdentity>> enumerate_supported_hid();
Result<std::vector<HidDeviceIdentity>> enumerate_supported_hid_with_paths();
}  // namespace openxhc
