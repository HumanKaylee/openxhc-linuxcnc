// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/hid_probe.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <variant>

#ifdef OPENXHC_HID_PROBE_FAKE_BACKEND
#include <hidapi/hidapi.h>

struct hid_device_ {
  std::size_t index;
};

namespace {
enum class FakeScenario { Success, NoDevices, InitFailure, OpenFailure, ProductFailure,
                          NonAsciiProduct };

FakeScenario scenario = FakeScenario::Success;
int init_calls{};
int exit_calls{};
int enumerate_calls{};
int free_calls{};
int open_calls{};
int close_calls{};
int manufacturer_calls{};
int product_calls{};
int serial_calls{};
unsigned short enumerated_vendor{};
unsigned short enumerated_product{};

std::array<hid_device_info, 6> fake_records{};
std::array<hid_device_, 6> fake_handles{};
std::array<std::array<char, 32>, 6> fake_paths{};

void reset_fake(FakeScenario next_scenario) {
  scenario = next_scenario;
  init_calls = 0;
  exit_calls = 0;
  enumerate_calls = 0;
  free_calls = 0;
  open_calls = 0;
  close_calls = 0;
  manufacturer_calls = 0;
  product_calls = 0;
  serial_calls = 0;
  enumerated_vendor = 0;
  enumerated_product = 0;

  constexpr std::array<std::string_view, 6> paths{
      "/dev/hidraw-fixture-1", "/dev/hidraw-wrong-vendor", "/dev/hidraw-wrong-pid",
      "/dev/hidraw-wrong-interface", "/dev/hidraw-fixture-0",
      "/dev/hidraw-wrong-product"};
  for (std::size_t index = 0; index < fake_records.size(); ++index) {
    std::fill(fake_paths[index].begin(), fake_paths[index].end(), '\0');
    std::copy(paths[index].begin(), paths[index].end(), fake_paths[index].begin());
    fake_handles[index].index = index;
    fake_records[index] = {};
    fake_records[index].path = fake_paths[index].data();
    fake_records[index].vendor_id = 0x10ce;
    fake_records[index].product_id = 0xeb73;
    fake_records[index].release_number = 0x0100;
    fake_records[index].interface_number = index == 0U ? 1 : 0;
    fake_records[index].next = index + 1U < fake_records.size()
                                   ? &fake_records[index + 1U]
                                   : nullptr;
  }
  fake_records[1].vendor_id = 0x10cf;
  fake_records[2].product_id = 0xeb93;
  fake_records[3].interface_number = 2;
}

int copy_wide(std::wstring_view input, wchar_t* output, std::size_t maximum) {
  if (maximum == 0U || input.size() >= maximum) {
    return -1;
  }
  std::copy(input.begin(), input.end(), output);
  output[input.size()] = L'\0';
  return 0;
}

template <class T>
bool has_error(const openxhc::Result<T>& result, openxhc::ErrorCode code,
               const char* message) {
  return std::holds_alternative<openxhc::Error>(result) &&
         std::get<openxhc::Error>(result).code == code &&
         std::get<openxhc::Error>(result).message == message;
}
}  // namespace

extern "C" int hid_init() {
  ++init_calls;
  return scenario == FakeScenario::InitFailure ? -1 : 0;
}

extern "C" int hid_exit() {
  ++exit_calls;
  return 0;
}

extern "C" hid_device_info* hid_enumerate(unsigned short vendor_id,
                                           unsigned short product_id) {
  ++enumerate_calls;
  enumerated_vendor = vendor_id;
  enumerated_product = product_id;
  return scenario == FakeScenario::NoDevices ? nullptr : fake_records.data();
}

extern "C" void hid_free_enumeration(hid_device_info*) { ++free_calls; }

extern "C" hid_device* hid_open_path(const char* path) {
  ++open_calls;
  if (scenario == FakeScenario::OpenFailure) {
    return nullptr;
  }
  for (std::size_t index = 0; index < fake_paths.size(); ++index) {
    if (std::string_view(path) == fake_paths[index].data()) {
      return &fake_handles[index];
    }
  }
  return nullptr;
}

extern "C" int hid_get_manufacturer_string(hid_device*, wchar_t* output,
                                            std::size_t maximum) {
  ++manufacturer_calls;
  return copy_wide(L"XHC", output, maximum);
}

extern "C" int hid_get_product_string(hid_device* device, wchar_t* output,
                                       std::size_t maximum) {
  ++product_calls;
  if (scenario == FakeScenario::ProductFailure) {
    return -1;
  }
  if (scenario == FakeScenario::NonAsciiProduct) {
    return copy_wide(L"XHC \u673a", output, maximum);
  }
  if (device->index == 5U) {
    return copy_wide(L"XHC MACH3 CAR", output, maximum);
  }
  return copy_wide(L"XHC MACH3 CARD", output, maximum);
}

extern "C" int hid_get_serial_number_string(hid_device*, wchar_t* output,
                                             std::size_t maximum) {
  ++serial_calls;
  return copy_wide(L"fixture-serial", output, maximum);
}

extern "C" void hid_close(hid_device*) { ++close_calls; }

int main() {
  reset_fake(FakeScenario::Success);
  const auto detailed = openxhc::enumerate_supported_hid_with_paths();
  CHECK(std::holds_alternative<std::vector<openxhc::HidDeviceIdentity>>(detailed));
  const auto& detailed_devices =
      std::get<std::vector<openxhc::HidDeviceIdentity>>(detailed);
  CHECK(detailed_devices.size() == 2U);
  CHECK(detailed_devices[0].identity.interface_number == 0);
  CHECK(detailed_devices[0].path == "/dev/hidraw-fixture-0");
  CHECK(detailed_devices[1].identity.interface_number == 1);
  CHECK(detailed_devices[1].path == "/dev/hidraw-fixture-1");
  CHECK(enumerated_vendor == 0x10ce);
  CHECK(enumerated_product == 0xeb73);
  CHECK(init_calls == 1);
  CHECK(exit_calls == 1);
  CHECK(enumerate_calls == 1);
  CHECK(free_calls == 1);
  CHECK(open_calls == 3);
  CHECK(close_calls == 3);
  CHECK(manufacturer_calls == 3);
  CHECK(product_calls == 3);
  CHECK(serial_calls == 3);

  reset_fake(FakeScenario::Success);
  const auto redacted = openxhc::enumerate_supported_hid();
  CHECK(std::holds_alternative<std::vector<openxhc::DeviceIdentity>>(redacted));
  const auto& redacted_devices = std::get<std::vector<openxhc::DeviceIdentity>>(redacted);
  CHECK(redacted_devices.size() == 2U);
  CHECK(redacted_devices[0].interface_number == 0);
  CHECK(redacted_devices[1].interface_number == 1);
  CHECK(exit_calls == 1);
  CHECK(free_calls == 1);
  CHECK(close_calls == 3);

  reset_fake(FakeScenario::NoDevices);
  CHECK(has_error(openxhc::enumerate_supported_hid(), openxhc::ErrorCode::UnsupportedDevice,
                  "No supported XHC HID interfaces found"));
  CHECK(init_calls == 1);
  CHECK(exit_calls == 1);
  CHECK(free_calls == 0);

  reset_fake(FakeScenario::InitFailure);
  CHECK(has_error(openxhc::enumerate_supported_hid(), openxhc::ErrorCode::Disconnected,
                  "Unable to initialize HIDAPI"));
  CHECK(init_calls == 1);
  CHECK(exit_calls == 0);
  CHECK(enumerate_calls == 0);

  reset_fake(FakeScenario::OpenFailure);
  CHECK(has_error(openxhc::enumerate_supported_hid(), openxhc::ErrorCode::Disconnected,
                  "Unable to open HID interface for descriptor query"));
  CHECK(exit_calls == 1);
  CHECK(free_calls == 1);
  CHECK(open_calls == 1);
  CHECK(close_calls == 0);

  reset_fake(FakeScenario::ProductFailure);
  CHECK(has_error(openxhc::enumerate_supported_hid(), openxhc::ErrorCode::Disconnected,
                  "Unable to query HID descriptors"));
  CHECK(exit_calls == 1);
  CHECK(free_calls == 1);
  CHECK(open_calls == 1);
  CHECK(close_calls == 1);

  reset_fake(FakeScenario::NonAsciiProduct);
  CHECK(has_error(openxhc::enumerate_supported_hid(), openxhc::ErrorCode::ParseError,
                  "HID product string is not printable ASCII"));
  CHECK(exit_calls == 1);
  CHECK(free_calls == 1);
  CHECK(open_calls == 1);
  CHECK(close_calls == 1);
  return 0;
}

#else

namespace {
template <class T>
bool has_error(const openxhc::Result<T>& result, openxhc::ErrorCode code,
               const char* message) {
  return std::holds_alternative<openxhc::Error>(result) &&
         std::get<openxhc::Error>(result).code == code &&
         std::get<openxhc::Error>(result).message == message;
}
}  // namespace

int main() {
  const auto first =
      openxhc::normalize_hid_identity(0x10ce, 0xeb73, 0, L"XHC MACH3 CARD", 0x0100);
  CHECK(std::holds_alternative<openxhc::DeviceIdentity>(first));
  CHECK(openxhc::is_supported_device(std::get<openxhc::DeviceIdentity>(first)));
  CHECK(std::get<openxhc::DeviceIdentity>(first).product_string == "XHC MACH3 CARD");

  const auto second =
      openxhc::normalize_hid_identity(0x10ce, 0xeb73, 1, L"XHC MACH3 CARD", 0x0100);
  CHECK(std::holds_alternative<openxhc::DeviceIdentity>(second));
  CHECK(openxhc::is_supported_device(std::get<openxhc::DeviceIdentity>(second)));

  const auto non_ascii =
      openxhc::normalize_hid_identity(0x10ce, 0xeb73, 0, L"XHC \u673a", 0x0100);
  CHECK(has_error(non_ascii, openxhc::ErrorCode::ParseError,
                  "HID product string is not printable ASCII"));
  const auto control_character =
      openxhc::normalize_hid_identity(0x10ce, 0xeb73, 0, L"XHC\nCARD", 0x0100);
  CHECK(has_error(control_character, openxhc::ErrorCode::ParseError,
                  "HID product string is not printable ASCII"));

  const auto wrong_vendor =
      openxhc::normalize_hid_identity(0x10cf, 0xeb73, 0, L"XHC MACH3 CARD", 0x0100);
  CHECK(std::holds_alternative<openxhc::DeviceIdentity>(wrong_vendor));
  CHECK(!openxhc::is_supported_device(std::get<openxhc::DeviceIdentity>(wrong_vendor)));
  const auto wrong_product_id =
      openxhc::normalize_hid_identity(0x10ce, 0xeb93, 0, L"XHC MACH3 CARD", 0x0100);
  CHECK(std::holds_alternative<openxhc::DeviceIdentity>(wrong_product_id));
  CHECK(!openxhc::is_supported_device(std::get<openxhc::DeviceIdentity>(wrong_product_id)));
  const auto wrong_product =
      openxhc::normalize_hid_identity(0x10ce, 0xeb73, 0, L"XHC MACH3 CAR", 0x0100);
  CHECK(std::holds_alternative<openxhc::DeviceIdentity>(wrong_product));
  CHECK(!openxhc::is_supported_device(std::get<openxhc::DeviceIdentity>(wrong_product)));
  const auto wrong_interface =
      openxhc::normalize_hid_identity(0x10ce, 0xeb73, 2, L"XHC MACH3 CARD", 0x0100);
  CHECK(std::holds_alternative<openxhc::DeviceIdentity>(wrong_interface));
  CHECK(!openxhc::is_supported_device(std::get<openxhc::DeviceIdentity>(wrong_interface)));

#ifndef OPENXHC_TEST_EXPECT_HIDAPI
  const auto enumeration = openxhc::enumerate_supported_hid();
  CHECK(has_error(enumeration, openxhc::ErrorCode::UnsupportedDevice,
                  "OpenXHC was built without HIDAPI"));
#endif
  return 0;
}
#endif
