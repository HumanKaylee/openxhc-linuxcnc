// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/hid_probe.hpp"
#include "openxhc/hid_transport.hpp"
#include "test_support.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {
using openxhc::DeviceIdentity;
using openxhc::Error;
using openxhc::ErrorCode;
using openxhc::HidTransport;
using openxhc::IHidBackend;
using openxhc::RawReport;
using openxhc::Result;

// One scripted backend step: either a byte count to deliver, or a negative error.
struct Step {
  int result;
  std::uint8_t fill;
  std::size_t length;
};

class ScriptedBackend final : public IHidBackend {
 public:
  explicit ScriptedBackend(std::vector<Step> steps) : steps_(std::move(steps)) {}

  int read(std::uint8_t* buffer, std::size_t capacity, int timeout_milliseconds) override {
    last_timeout_ = timeout_milliseconds;
    ++reads_;
    if (index_ >= steps_.size()) {
      return -1;
    }
    const Step step = steps_[index_++];
    if (step.result > 0) {
      const std::size_t length = step.length < capacity ? step.length : capacity;
      for (std::size_t i = 0; i < length; ++i) {
        buffer[i] = step.fill;
      }
    }
    return step.result;
  }

  void close() noexcept override { ++closes_; }

  int last_timeout() const noexcept { return last_timeout_; }
  int reads() const noexcept { return reads_; }
  int closes() const noexcept { return closes_; }

 private:
  std::vector<Step> steps_;
  std::size_t index_{};
  int last_timeout_{-1};
  int reads_{};
  int closes_{};
};

DeviceIdentity supported_identity() {
  return DeviceIdentity{0x10ce, 0xeb73, 0, "XHC MACH3 CARD", 0x0100};
}

template <class T>
bool has_error(const Result<T>& result, ErrorCode code) {
  return std::holds_alternative<Error>(result) && std::get<Error>(result).code == code;
}
}  // namespace

int main() {
  // A well-formed 38-byte record is delivered intact.
  {
    auto backend = std::make_unique<ScriptedBackend>(
        std::vector<Step>{Step{38, 0xa5U, 38U}});
    auto* raw = backend.get();
    HidTransport transport{std::move(backend), supported_identity()};

    auto identity = transport.identify();
    CHECK(std::holds_alternative<DeviceIdentity>(identity));
    CHECK(std::get<DeviceIdentity>(identity).product_id == 0xeb73);

    auto record = transport.read(std::chrono::milliseconds{250});
    CHECK(std::holds_alternative<RawReport>(record));
    CHECK(std::get<RawReport>(record).size == 38U);
    CHECK(std::get<RawReport>(record).bytes[0] == 0xa5U);
    CHECK(std::get<RawReport>(record).bytes[37U] == 0xa5U);
    CHECK(raw->last_timeout() == 250);
  }

  // NEGATIVE: writing is refused and performs no backend I/O at all.
  {
    auto backend = std::make_unique<ScriptedBackend>(std::vector<Step>{});
    auto* raw = backend.get();
    HidTransport transport{std::move(backend), supported_identity()};

    RawReport outgoing{};
    outgoing.size = 4U;
    const auto status = transport.write(outgoing);
    CHECK(!status.success);
    CHECK(status.error.has_value());
    CHECK(status.error->code == ErrorCode::WriteDisabled);
    CHECK(raw->reads() == 0);
  }

  // NEGATIVE: a timeout is reported as a timeout, not as an empty record.
  {
    auto backend = std::make_unique<ScriptedBackend>(std::vector<Step>{Step{0, 0U, 0U}});
    HidTransport transport{std::move(backend), supported_identity()};
    CHECK(has_error(transport.read(std::chrono::milliseconds{10}), ErrorCode::Timeout));
  }

  // NEGATIVE: a backend error is a disconnect, not a short record.
  {
    auto backend = std::make_unique<ScriptedBackend>(std::vector<Step>{Step{-1, 0U, 0U}});
    HidTransport transport{std::move(backend), supported_identity()};
    CHECK(has_error(transport.read(std::chrono::milliseconds{10}), ErrorCode::Disconnected));
  }

  // NEGATIVE: a record larger than the fixed capacity is rejected, not truncated.
  {
    auto backend = std::make_unique<ScriptedBackend>(std::vector<Step>{Step{65, 0x11U, 64U}});
    HidTransport transport{std::move(backend), supported_identity()};
    CHECK(has_error(transport.read(std::chrono::milliseconds{10}), ErrorCode::InvalidReport));
  }

  // A 64-byte record sits exactly on the capacity boundary and is accepted.
  {
    auto backend = std::make_unique<ScriptedBackend>(std::vector<Step>{Step{64, 0x22U, 64U}});
    HidTransport transport{std::move(backend), supported_identity()};
    auto record = transport.read(std::chrono::milliseconds{10});
    CHECK(std::holds_alternative<RawReport>(record));
    CHECK(std::get<RawReport>(record).size == 64U);
    CHECK(std::get<RawReport>(record).bytes[63U] == 0x22U);
  }

  // Reading after close fails, and close is idempotent.
  {
    auto backend = std::make_unique<ScriptedBackend>(
        std::vector<Step>{Step{38, 0x01U, 38U}, Step{38, 0x01U, 38U}});
    auto* raw = backend.get();
    HidTransport transport{std::move(backend), supported_identity()};
    CHECK(std::holds_alternative<RawReport>(transport.read(std::chrono::milliseconds{10})));
    transport.close();
    transport.close();
    CHECK(transport.closed());
    CHECK(raw->closes() == 1);
    CHECK(has_error(transport.read(std::chrono::milliseconds{10}), ErrorCode::Disconnected));
    // NEGATIVE: identify must also fail once closed rather than report stale identity.
    CHECK(has_error(transport.identify(), ErrorCode::Disconnected));
    // Writing after close is still a write refusal, never a disconnect.
    RawReport outgoing{};
    outgoing.size = 1U;
    CHECK(transport.write(outgoing).error->code == ErrorCode::WriteDisabled);
  }

  // Identity string source. This controller carries "XHC MACH3 CARD" in its MANUFACTURER
  // descriptor and has no product string at all, so a product-only rule never matches it
  // on a host that does not synthesise one.
  {
    const wchar_t* product = L"SOME PRODUCT";
    const wchar_t* manufacturer = L"XHC MACH3 CARD";
    const wchar_t* empty = L"";

    // A real product string wins when the device provides one.
    CHECK(openxhc::select_identity_string(product, manufacturer) == std::wstring_view(product));
    // The observed case: no product descriptor, identity in the manufacturer descriptor.
    CHECK(openxhc::select_identity_string(nullptr, manufacturer) ==
          std::wstring_view(manufacturer));
    // NEGATIVE: an EMPTY product string must not win over a real manufacturer string.
    CHECK(openxhc::select_identity_string(empty, manufacturer) ==
          std::wstring_view(manufacturer));
    // NEGATIVE: nothing usable anywhere yields empty, not a dereference of null.
    CHECK(openxhc::select_identity_string(nullptr, nullptr).empty());
    CHECK(openxhc::select_identity_string(empty, empty).empty());
    CHECK(openxhc::select_identity_string(empty, nullptr).empty());
    // A product string with no manufacturer still works.
    CHECK(openxhc::select_identity_string(product, nullptr) == std::wstring_view(product));
  }

  // Reconnect policy: only a lost link warrants reopening. A timeout means keep reading
  // on the same handle, and a malformed record is a data fault that reopening would hide.
  CHECK(openxhc::is_recoverable_read_error(ErrorCode::Disconnected));
  CHECK(!openxhc::is_recoverable_read_error(ErrorCode::Timeout));
  CHECK(!openxhc::is_recoverable_read_error(ErrorCode::InvalidReport));
  CHECK(!openxhc::is_recoverable_read_error(ErrorCode::UnsupportedDevice));
  CHECK(!openxhc::is_recoverable_read_error(ErrorCode::WriteDisabled));
  CHECK(!openxhc::is_recoverable_read_error(ErrorCode::InvalidArgument));
  CHECK(!openxhc::is_recoverable_read_error(ErrorCode::InvalidTransition));
  CHECK(!openxhc::is_recoverable_read_error(ErrorCode::ParseError));

  return 0;
}
