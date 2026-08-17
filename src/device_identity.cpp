// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/device_identity.hpp"

namespace openxhc {
bool is_supported_device(const DeviceIdentity& identity) noexcept {
  const bool supported_interface =
      identity.interface_number == 0 || identity.interface_number == 1;
  return identity.vendor_id == 0x10ce && identity.product_id == 0xeb73 &&
         supported_interface && identity.product_string == "XHC MACH3 CARD";
}
}
