// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/state_machine.hpp"
#include "test_support.hpp"

int main() {
  using openxhc::DeviceState;
  using openxhc::Error;
  using openxhc::ErrorCode;
  using openxhc::StateMachine;

  StateMachine state;
  CHECK(state.current() == DeviceState::Disconnected);

  const auto disconnected_self = state.transition(DeviceState::Disconnected);
  CHECK(!disconnected_self.success);
  CHECK(disconnected_self.error.has_value());
  CHECK(disconnected_self.error->code == ErrorCode::InvalidTransition);

  CHECK(state.transition(DeviceState::Identified).success);
  CHECK(state.transition(DeviceState::Negotiating).success);
  CHECK(state.transition(DeviceState::ReadyInhibited).success);
  const auto ready_to_streaming = state.transition(DeviceState::Streaming);
  CHECK(!ready_to_streaming.success);
  CHECK(ready_to_streaming.error.has_value());
  CHECK(ready_to_streaming.error->code == ErrorCode::InvalidTransition);
  CHECK(state.transition(DeviceState::Armed).success);
  CHECK(state.transition(DeviceState::Streaming).success);
  CHECK(state.transition(DeviceState::Holding).success);
  CHECK(state.transition(DeviceState::Streaming).success);
  CHECK(state.transition(DeviceState::ReadyInhibited).success);
  CHECK(state.transition(DeviceState::Disconnected).success);

  CHECK(state.transition(DeviceState::Identified).success);
  CHECK(state.transition(DeviceState::Disconnected).success);
  CHECK(state.transition(DeviceState::Identified).success);
  CHECK(state.transition(DeviceState::Negotiating).success);
  CHECK(state.transition(DeviceState::Disconnected).success);
  CHECK(state.transition(DeviceState::Identified).success);
  CHECK(state.transition(DeviceState::Negotiating).success);
  CHECK(state.transition(DeviceState::ReadyInhibited).success);
  CHECK(state.transition(DeviceState::Armed).success);

  StateMachine armed_to_ready;
  CHECK(armed_to_ready.transition(DeviceState::Identified).success);
  CHECK(armed_to_ready.transition(DeviceState::Negotiating).success);
  CHECK(armed_to_ready.transition(DeviceState::ReadyInhibited).success);
  CHECK(armed_to_ready.transition(DeviceState::Armed).success);
  CHECK(armed_to_ready.transition(DeviceState::ReadyInhibited).success);

  StateMachine armed_to_disconnected;
  CHECK(armed_to_disconnected.transition(DeviceState::Identified).success);
  CHECK(armed_to_disconnected.transition(DeviceState::Negotiating).success);
  CHECK(armed_to_disconnected.transition(DeviceState::ReadyInhibited).success);
  CHECK(armed_to_disconnected.transition(DeviceState::Armed).success);
  CHECK(armed_to_disconnected.transition(DeviceState::Disconnected).success);

  StateMachine streaming_to_disconnected;
  CHECK(streaming_to_disconnected.transition(DeviceState::Identified).success);
  CHECK(streaming_to_disconnected.transition(DeviceState::Negotiating).success);
  CHECK(streaming_to_disconnected.transition(DeviceState::ReadyInhibited).success);
  CHECK(streaming_to_disconnected.transition(DeviceState::Armed).success);
  CHECK(streaming_to_disconnected.transition(DeviceState::Streaming).success);
  CHECK(streaming_to_disconnected.transition(DeviceState::Disconnected).success);

  StateMachine holding_to_ready;
  CHECK(holding_to_ready.transition(DeviceState::Identified).success);
  CHECK(holding_to_ready.transition(DeviceState::Negotiating).success);
  CHECK(holding_to_ready.transition(DeviceState::ReadyInhibited).success);
  CHECK(holding_to_ready.transition(DeviceState::Armed).success);
  CHECK(holding_to_ready.transition(DeviceState::Streaming).success);
  CHECK(holding_to_ready.transition(DeviceState::Holding).success);
  CHECK(holding_to_ready.transition(DeviceState::ReadyInhibited).success);

  StateMachine holding_to_disconnected;
  CHECK(holding_to_disconnected.transition(DeviceState::Identified).success);
  CHECK(holding_to_disconnected.transition(DeviceState::Negotiating).success);
  CHECK(holding_to_disconnected.transition(DeviceState::ReadyInhibited).success);
  CHECK(holding_to_disconnected.transition(DeviceState::Armed).success);
  CHECK(holding_to_disconnected.transition(DeviceState::Streaming).success);
  CHECK(holding_to_disconnected.transition(DeviceState::Holding).success);
  CHECK(holding_to_disconnected.transition(DeviceState::Disconnected).success);

  const Error first_error{ErrorCode::Timeout, "status stale"};
  state.fault(first_error);
  CHECK(state.current() == DeviceState::Faulted);
  CHECK(state.first_fault() != nullptr);
  CHECK(state.first_fault()->code == ErrorCode::Timeout);
  CHECK(state.first_fault()->message == "status stale");
  CHECK(state.fault_count() == 1U);

  const auto direct_fault_exit = state.transition(DeviceState::ReadyInhibited);
  CHECK(!direct_fault_exit.success);
  CHECK(direct_fault_exit.error.has_value());
  CHECK(direct_fault_exit.error->code == ErrorCode::InvalidTransition);
  CHECK(state.current() == DeviceState::Faulted);

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
