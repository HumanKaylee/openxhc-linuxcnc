// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <string>

namespace openxhc {
struct DeviceIdentity {
  std::uint16_t vendor_id;
  std::uint16_t product_id;
  int interface_number;
  // The device's identity string. Deliberately NOT called a product string: this
  // controller exposes no product string descriptor, and the value is read from its
  // MANUFACTURER descriptor. See select_identity_string() in hid_probe.hpp.
  //
  // Dynamically allocated enumeration control-plane text; keep it out of future
  // fixed-capacity motion paths.
  std::string identity_string;
  std::uint16_t release_number;
};
bool is_supported_device(const DeviceIdentity& identity) noexcept;
}
