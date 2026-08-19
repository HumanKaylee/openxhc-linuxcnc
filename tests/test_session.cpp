// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/session.hpp"
#include "test_support.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace {
using openxhc::DeviceIdentity;
using openxhc::DeviceState;
using openxhc::Error;
using openxhc::ErrorCode;
using openxhc::ITransport;
using openxhc::RawReport;
using openxhc::ReadOnlySession;
using openxhc::Result;
using openxhc::Status;

RawReport status_record(std::uint8_t leading, std::size_t size, std::uint8_t vary_at,
                        std::uint8_t vary_to) {
  RawReport report{};
  report.size = size;
  if (size > 0U) {
    report.bytes[0] = leading;
  }
  // Offset 0 is the leading identity byte; varying it would silently turn every
  // "good record" in these tests into a malformed one. Treat 0 as "vary nothing".
  if (vary_at != 0U && vary_at < size) {
    report.bytes[vary_at] = vary_to;
  }
  return report;
}

// Scripted transport: each step is either a record or an error to return from read().
struct ReadStep {
  bool ok;
  RawReport report;
  ErrorCode code;
};

class ScriptedTransport final : public ITransport {
 public:
  ScriptedTransport(std::vector<ReadStep> steps, bool identifies)
      : steps_(std::move(steps)), identifies_(identifies) {}

  Result<DeviceIdentity> identify() override {
    ++identify_calls_;
    if (!identifies_) {
      return Error{ErrorCode::UnsupportedDevice, "not the supported controller"};
    }
    return DeviceIdentity{0x10ce, 0xeb73, 0, "XHC MACH3 CARD", 0x0100};
  }

  Result<RawReport> read(std::chrono::milliseconds) override {
    if (index_ >= steps_.size()) {
      return Error{ErrorCode::Disconnected, "exhausted"};
    }
    const ReadStep step = steps_[index_++];
    if (step.ok) {
      return step.report;
    }
    return Error{step.code, "scripted"};
  }

  Status write(const RawReport&) override {
    ++write_calls_;
    return Status::fail(ErrorCode::WriteDisabled, "scripted transport refuses writes");
  }

  void close() noexcept override { ++close_calls_; }

  int identify_calls() const noexcept { return identify_calls_; }
  int write_calls() const noexcept { return write_calls_; }
  int close_calls() const noexcept { return close_calls_; }

 private:
  std::vector<ReadStep> steps_;
  bool identifies_;
  std::size_t index_{};
  int identify_calls_{};
  int write_calls_{};
  int close_calls_{};
};

std::unique_ptr<ScriptedTransport> make(std::vector<ReadStep> steps, bool identifies = true) {
  return std::make_unique<ScriptedTransport>(std::move(steps), identifies);
}
}  // namespace

int main() {
  // Opening walks Disconnected -> Identified -> Negotiating -> ReadyInhibited and STOPS.
  {
    auto transport = make({});
    auto* raw = transport.get();
    ReadOnlySession session{std::move(transport)};
    CHECK(session.state() == DeviceState::Disconnected);
    CHECK(session.open().success);
    CHECK(session.state() == DeviceState::ReadyInhibited);
    CHECK(raw->identify_calls() == 1);
    // The session must never arm itself. Armed is where motion authority would begin.
    CHECK(session.state() != DeviceState::Armed);
    CHECK(session.state() != DeviceState::Streaming);
  }

  // NEGATIVE: a device that does not identify never leaves Disconnected.
  {
    auto session = ReadOnlySession{make({}, false)};
    const auto status = session.open();
    CHECK(!status.success);
    CHECK(status.error->code == ErrorCode::UnsupportedDevice);
    CHECK(session.state() == DeviceState::Disconnected);
  }

  // NEGATIVE: polling before open is refused, and does not consume a read.
  {
    auto session = ReadOnlySession{make({{true, status_record(0x04U, 38U, 0U, 0U), {}}})};
    const auto status = session.poll(std::chrono::milliseconds{5});
    CHECK(!status.success);
    CHECK(status.error->code == ErrorCode::InvalidTransition);
    CHECK(session.records() == 0U);
  }

  // Records accumulate and drive activity tracking.
  {
    auto session = ReadOnlySession{make({
        {true, status_record(0x04U, 38U, 9U, 0x00U), {}},
        {true, status_record(0x04U, 38U, 9U, 0x00U), {}},
        {true, status_record(0x04U, 38U, 9U, 0x7fU), {}},
    })};
    CHECK(session.open().success);
    CHECK(session.poll(std::chrono::milliseconds{5}).success);
    CHECK(session.poll(std::chrono::milliseconds{5}).success);
    CHECK(session.poll(std::chrono::milliseconds{5}).success);
    CHECK(session.records() == 3U);
    CHECK(session.activity().observations() == 3U);
    CHECK(session.activity().changed(9U));
    CHECK(session.activity().changed_count() == 1U);
    CHECK(session.state() == DeviceState::ReadyInhibited);
  }

  // A malformed record is counted, not faulted, and does not pollute activity.
  {
    auto session = ReadOnlySession{make({
        {true, status_record(0x04U, 38U, 0U, 0U), {}},
        {true, status_record(0x99U, 38U, 0U, 0U), {}},  // wrong leading byte
        {true, status_record(0x04U, 12U, 0U, 0U), {}},  // wrong length
    })};
    CHECK(session.open().success);
    CHECK(session.poll(std::chrono::milliseconds{5}).success);
    CHECK(!session.poll(std::chrono::milliseconds{5}).success);
    CHECK(!session.poll(std::chrono::milliseconds{5}).success);
    CHECK(session.records() == 1U);
    CHECK(session.malformed() == 2U);
    CHECK(session.activity().observations() == 1U);
    // NEGATIVE: malformed input must not fault the lifecycle.
    CHECK(session.state() == DeviceState::ReadyInhibited);
    CHECK(session.machine().fault_count() == 0U);
  }

  // A timeout is not an error state and not a record.
  {
    auto session = ReadOnlySession{make({{false, {}, ErrorCode::Timeout}})};
    CHECK(session.open().success);
    const auto status = session.poll(std::chrono::milliseconds{5});
    CHECK(!status.success);
    CHECK(status.error->code == ErrorCode::Timeout);
    CHECK(session.state() == DeviceState::ReadyInhibited);
    CHECK(session.records() == 0U);
    CHECK(session.machine().fault_count() == 0U);
  }

  // A disconnect returns to Disconnected WITHOUT faulting - the periodic reset on this
  // controller is the expected environment - and the session can walk forward again.
  {
    auto session = ReadOnlySession{make({
        {true, status_record(0x04U, 38U, 0U, 0U), {}},
        {false, {}, ErrorCode::Disconnected},
        {true, status_record(0x04U, 38U, 0U, 0U), {}},
    })};
    CHECK(session.open().success);
    CHECK(session.poll(std::chrono::milliseconds{5}).success);
    CHECK(!session.poll(std::chrono::milliseconds{5}).success);
    CHECK(session.handle_disconnect().success);
    CHECK(session.state() == DeviceState::Disconnected);
    CHECK(session.machine().fault_count() == 0U);
    CHECK(session.open().success);
    CHECK(session.state() == DeviceState::ReadyInhibited);
    CHECK(session.poll(std::chrono::milliseconds{5}).success);
    CHECK(session.records() == 2U);
    // NEGATIVE: reconnecting must not have armed anything.
    CHECK(session.state() != DeviceState::Armed);
  }

  // The session never writes, and closing is idempotent.
  {
    auto transport = make({});
    auto* raw = transport.get();
    ReadOnlySession session{std::move(transport)};
    CHECK(session.open().success);
    session.close();
    session.close();
    CHECK(raw->close_calls() == 1);
    CHECK(raw->write_calls() == 0);
    // NEGATIVE: polling after close is refused.
    CHECK(!session.poll(std::chrono::milliseconds{5}).success);
  }

  return 0;
}
