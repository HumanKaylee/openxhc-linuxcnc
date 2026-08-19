// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/session.hpp"

#include "openxhc/device_identity.hpp"

#include <utility>
#include <variant>

namespace openxhc {
ReadOnlySession::ReadOnlySession(std::unique_ptr<ITransport> transport) noexcept
    : transport_(std::move(transport)) {}

Status ReadOnlySession::open() {
  if (closed_) {
    return Status::fail(ErrorCode::Disconnected, "session is closed");
  }
  if (transport_ == nullptr) {
    return Status::fail(ErrorCode::Disconnected, "session has no transport");
  }
  if (machine_.current() != DeviceState::Disconnected) {
    return Status::fail(ErrorCode::InvalidTransition, "session is already open");
  }

  auto identity = transport_->identify();
  if (std::holds_alternative<Error>(identity)) {
    // Stay Disconnected. An unidentified device must not advance the lifecycle.
    return Status::fail(std::get<Error>(identity).code, "device did not identify");
  }
  if (!is_supported_device(std::get<DeviceIdentity>(identity))) {
    return Status::fail(ErrorCode::UnsupportedDevice, "device is not the supported controller");
  }

  Status step = machine_.transition(DeviceState::Identified);
  if (!step.success) {
    return step;
  }
  step = machine_.transition(DeviceState::Negotiating);
  if (!step.success) {
    return step;
  }
  // Stops here, deliberately. Armed is where motion authority would begin and nothing
  // in this project has earned it.
  return machine_.transition(DeviceState::ReadyInhibited);
}

Status ReadOnlySession::poll(std::chrono::milliseconds timeout) {
  if (closed_) {
    return Status::fail(ErrorCode::Disconnected, "session is closed");
  }
  if (transport_ == nullptr) {
    return Status::fail(ErrorCode::Disconnected, "session has no transport");
  }
  if (machine_.current() != DeviceState::ReadyInhibited) {
    return Status::fail(ErrorCode::InvalidTransition, "session is not ready to poll");
  }

  auto incoming = transport_->read(timeout);
  if (std::holds_alternative<Error>(incoming)) {
    const Error error = std::get<Error>(incoming);
    // A timeout and a lost link are both ordinary on this controller; neither is a fault.
    return Status::fail(error.code, error.message.c_str());
  }

  auto parsed = parse_status_record(std::get<RawReport>(incoming));
  if (std::holds_alternative<Error>(parsed)) {
    // A record we do not recognise is counted and reported, not faulted: the device
    // profile is incomplete by design and an unknown record is evidence, not an error.
    ++malformed_;
    return Status::fail(ErrorCode::InvalidReport, "unrecognised status record");
  }

  ++records_;
  activity_.observe(std::get<StatusRecord>(parsed));
  return Status::ok();
}

Status ReadOnlySession::handle_disconnect() {
  if (machine_.current() == DeviceState::Disconnected) {
    return Status::ok();
  }
  return machine_.transition(DeviceState::Disconnected);
}

void ReadOnlySession::close() noexcept {
  if (closed_) {
    return;
  }
  closed_ = true;
  if (transport_ != nullptr) {
    // Close it; do NOT destroy it. Releasing the transport here would invalidate any
    // observer the caller holds, and the session owns it until the session itself dies.
    transport_->close();
  }
}
}  // namespace openxhc
