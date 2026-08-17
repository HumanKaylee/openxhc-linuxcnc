// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/state_machine.hpp"
#include "test_support.hpp"

#include <array>

namespace {
using openxhc::DeviceState;
using openxhc::StateMachine;

constexpr std::array kStates{DeviceState::Disconnected, DeviceState::Identified,
                             DeviceState::Negotiating, DeviceState::ReadyInhibited,
                             DeviceState::Armed, DeviceState::Streaming, DeviceState::Holding,
                             DeviceState::Faulted};

bool is_approved_ordinary_transition(DeviceState current, DeviceState next) {
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

bool prepare_state(StateMachine& state, DeviceState target) {
  switch (target) {
    case DeviceState::Disconnected:
      return true;
    case DeviceState::Identified:
      return state.transition(DeviceState::Identified).success;
    case DeviceState::Negotiating:
      return state.transition(DeviceState::Identified).success &&
             state.transition(DeviceState::Negotiating).success;
    case DeviceState::ReadyInhibited:
      return state.transition(DeviceState::Identified).success &&
             state.transition(DeviceState::Negotiating).success &&
             state.transition(DeviceState::ReadyInhibited).success;
    case DeviceState::Armed:
      return state.transition(DeviceState::Identified).success &&
             state.transition(DeviceState::Negotiating).success &&
             state.transition(DeviceState::ReadyInhibited).success &&
             state.transition(DeviceState::Armed).success;
    case DeviceState::Streaming:
      return state.transition(DeviceState::Identified).success &&
             state.transition(DeviceState::Negotiating).success &&
             state.transition(DeviceState::ReadyInhibited).success &&
             state.transition(DeviceState::Armed).success &&
             state.transition(DeviceState::Streaming).success;
    case DeviceState::Holding:
      return state.transition(DeviceState::Identified).success &&
             state.transition(DeviceState::Negotiating).success &&
             state.transition(DeviceState::ReadyInhibited).success &&
             state.transition(DeviceState::Armed).success &&
             state.transition(DeviceState::Streaming).success &&
             state.transition(DeviceState::Holding).success;
    case DeviceState::Faulted:
      return false;
  }
  return false;
}
}  // namespace

int main() {
  using openxhc::Error;
  using openxhc::ErrorCode;

  StateMachine initial;
  CHECK(initial.current() == DeviceState::Disconnected);

  for (const DeviceState current : kStates) {
    if (current == DeviceState::Faulted) {
      continue;
    }
    for (const DeviceState next : kStates) {
      StateMachine state;
      CHECK(prepare_state(state, current));
      CHECK(state.current() == current);

      const bool allowed = is_approved_ordinary_transition(current, next);
      const auto result = state.transition(next);
      CHECK(result.success == allowed);
      if (allowed) {
        CHECK(state.current() == next);
      } else {
        CHECK(result.error.has_value());
        CHECK(result.error->code == ErrorCode::InvalidTransition);
        CHECK(state.current() == current);
      }
    }
  }

  StateMachine faulted;
  faulted.fault(Error{ErrorCode::Timeout, "status stale"});
  for (const DeviceState next : kStates) {
    const auto result = faulted.transition(next);
    CHECK(!result.success);
    CHECK(result.error.has_value());
    CHECK(result.error->code == ErrorCode::InvalidTransition);
    CHECK(faulted.current() == DeviceState::Faulted);
  }

  for (const DeviceState current : kStates) {
    if (current == DeviceState::Faulted) {
      continue;
    }
    StateMachine state;
    CHECK(prepare_state(state, current));
    const auto result = state.acknowledge_fault(true, true);
    CHECK(!result.success);
    CHECK(result.error.has_value());
    CHECK(result.error->code == ErrorCode::InvalidTransition);
    CHECK(state.current() == current);
  }

  for (const DeviceState current : kStates) {
    if (current == DeviceState::Faulted) {
      continue;
    }
    StateMachine state;
    CHECK(prepare_state(state, current));
    state.fault(Error{ErrorCode::Timeout, "status stale"});
    CHECK(state.current() == DeviceState::Faulted);
    CHECK(state.first_fault() != nullptr);
    CHECK(state.first_fault()->code == ErrorCode::Timeout);
    CHECK(state.first_fault()->message == "status stale");
    CHECK(state.fault_count() == 1U);
  }

  StateMachine state;
  CHECK(prepare_state(state, DeviceState::Armed));
  state.fault(Error{ErrorCode::Timeout, "status stale"});
  CHECK(state.current() == DeviceState::Faulted);
  CHECK(state.first_fault() != nullptr);
  CHECK(state.first_fault()->code == ErrorCode::Timeout);
  CHECK(state.first_fault()->message == "status stale");
  CHECK(state.fault_count() == 1U);

  const auto unhealthy_acknowledgement = state.acknowledge_fault(false, true);
  CHECK(!unhealthy_acknowledgement.success);
  CHECK(unhealthy_acknowledgement.error.has_value());
  CHECK(unhealthy_acknowledgement.error->code == ErrorCode::InvalidTransition);
  CHECK(state.current() == DeviceState::Faulted);

  const auto enabled_outputs_acknowledgement = state.acknowledge_fault(true, false);
  CHECK(!enabled_outputs_acknowledgement.success);
  CHECK(enabled_outputs_acknowledgement.error.has_value());
  CHECK(enabled_outputs_acknowledgement.error->code == ErrorCode::InvalidTransition);
  CHECK(state.current() == DeviceState::Faulted);

  state.fault(Error{ErrorCode::Disconnected, "cable removed"});
  CHECK(state.current() == DeviceState::Faulted);
  CHECK(state.first_fault() != nullptr);
  CHECK(state.first_fault()->code == ErrorCode::Timeout);
  CHECK(state.first_fault()->message == "status stale");
  CHECK(state.fault_count() == 2U);

  CHECK(state.acknowledge_fault(true, true).success);
  CHECK(state.current() == DeviceState::ReadyInhibited);
  CHECK(state.first_fault() == nullptr);
  CHECK(state.fault_count() == 2U);
  return 0;
}
