// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/state_machine.hpp"

#include <utility>

namespace openxhc {
namespace {
bool is_allowed_transition(DeviceState current, DeviceState next) noexcept {
  switch (current) {
    case DeviceState::Disconnected:
      return next == DeviceState::Identified;
    case DeviceState::Identified:
      return next == DeviceState::Negotiating || next == DeviceState::Disconnected;
    case DeviceState::Negotiating:
      return next == DeviceState::ReadyInhibited || next == DeviceState::Disconnected;
    case DeviceState::ReadyInhibited:
      return next == DeviceState::Armed || next == DeviceState::Disconnected;
    case DeviceState::Armed:
      return next == DeviceState::Streaming || next == DeviceState::ReadyInhibited ||
             next == DeviceState::Disconnected;
    case DeviceState::Streaming:
      return next == DeviceState::Holding || next == DeviceState::ReadyInhibited ||
             next == DeviceState::Disconnected;
    case DeviceState::Holding:
      return next == DeviceState::Streaming || next == DeviceState::ReadyInhibited ||
             next == DeviceState::Disconnected;
    case DeviceState::Faulted:
      return false;
  }
  return false;
}
}  // namespace

DeviceState StateMachine::current() const noexcept { return current_; }

Status StateMachine::transition(DeviceState next) {
  if (!is_allowed_transition(current_, next)) {
    return Status::fail(ErrorCode::InvalidTransition, "device lifecycle transition is not allowed");
  }

  current_ = next;
  return Status::ok();
}

void StateMachine::fault(Error error) {
  ++fault_count_;
  if (!first_fault_.has_value()) {
    first_fault_ = std::move(error);
  }
  current_ = DeviceState::Faulted;
}

Status StateMachine::acknowledge_fault(bool communication_healthy, bool outputs_inhibited) {
  if (current_ != DeviceState::Faulted || !communication_healthy || !outputs_inhibited) {
    return Status::fail(ErrorCode::InvalidTransition,
                        "fault acknowledgement requires healthy communication and inhibited outputs");
  }

  first_fault_.reset();
  current_ = DeviceState::ReadyInhibited;
  return Status::ok();
}

const Error* StateMachine::first_fault() const noexcept {
  return first_fault_ ? &*first_fault_ : nullptr;
}

std::uint64_t StateMachine::fault_count() const noexcept { return fault_count_; }
}  // namespace openxhc
