// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/device_identity.hpp"
#include "test_support.hpp"

int main() {
  using openxhc::DeviceIdentity;
  CHECK(openxhc::is_supported_device(DeviceIdentity{0x10ce, 0xeb73, 0, "XHC MACH3 CARD", 0x0100}));
  CHECK(openxhc::is_supported_device(DeviceIdentity{0x10ce, 0xeb73, 1, "XHC MACH3 CARD", 0x0100}));
  CHECK(!openxhc::is_supported_device(DeviceIdentity{0x10ce, 0xeb73, 2, "XHC MACH3 CARD", 0x0100}));
  CHECK(!openxhc::is_supported_device(DeviceIdentity{0x10ce, 0xeb93, 0, "XHC MACH3 CARD", 0x0100}));
  CHECK(!openxhc::is_supported_device(DeviceIdentity{0x10ce, 0xeb73, 0, "XHC WHB04B-6", 0x0100}));
  return 0;
}
