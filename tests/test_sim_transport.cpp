// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/sim_transport.hpp"
#include "test_support.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <variant>

namespace {
template <class T>
bool has_error(const openxhc::Result<T>& result, openxhc::ErrorCode code,
               const char* message) {
  if (!std::holds_alternative<openxhc::Error>(result)) {
    return false;
  }
  const auto& error = std::get<openxhc::Error>(result);
  return error.code == code && error.message == message;
}

bool has_error(const openxhc::Status& status, openxhc::ErrorCode code, const char* message) {
  return !status.success && status.error.has_value() && status.error->code == code &&
         status.error->message == message;
}
}  // namespace

int main() {
  using namespace std::chrono_literals;

  openxhc::ManualClock clock;
  openxhc::SimTransport transport(clock);

  const auto identity = transport.identify();
  CHECK(std::holds_alternative<openxhc::DeviceIdentity>(identity));
  const auto& device = std::get<openxhc::DeviceIdentity>(identity);
  CHECK(device.vendor_id == 0x10ce);
  CHECK(device.product_id == 0xeb73);
  CHECK(device.interface_number == 0);
  CHECK(device.identity_string == "XHC MACH3 CARD");
  CHECK(device.release_number == 0x0100);

  const std::array<std::uint8_t, 2> bytes{0x04, 0x7e};
  const auto parsed = openxhc::parse_raw_report(bytes);
  CHECK(std::holds_alternative<openxhc::RawReport>(parsed));
  transport.enqueue_read(std::get<openxhc::RawReport>(parsed));
  const auto read = transport.read(10ms);
  CHECK(std::holds_alternative<openxhc::RawReport>(read));
  CHECK(std::get<openxhc::RawReport>(read).size == 2U);
  CHECK(std::get<openxhc::RawReport>(read).bytes[0] == 0x04);
  CHECK(std::get<openxhc::RawReport>(read).bytes[1] == 0x7e);

  transport.enqueue_error({openxhc::ErrorCode::Disconnected, "scripted disconnect"});
  CHECK(has_error(transport.read(10ms), openxhc::ErrorCode::Disconnected,
                  "scripted disconnect"));
  CHECK(has_error(transport.read(10ms), openxhc::ErrorCode::Timeout, "script exhausted"));

  const auto write_report = std::get<openxhc::RawReport>(parsed);
  CHECK(transport.write(write_report).success);
  CHECK(transport.writes().size() == 1U);
  CHECK(transport.writes()[0].size == 2U);
  CHECK(transport.writes()[0].bytes[1] == 0x7e);

  clock.advance(25ms);
  CHECK(clock.now() == 25ms);

  transport.close();
  transport.close();
  CHECK(has_error(transport.identify(), openxhc::ErrorCode::Disconnected, "transport closed"));
  CHECK(has_error(transport.read(10ms), openxhc::ErrorCode::Disconnected, "transport closed"));
  CHECK(has_error(transport.write(write_report), openxhc::ErrorCode::Disconnected,
                  "transport closed"));
  CHECK(transport.writes().size() == 1U);
  return 0;
}
