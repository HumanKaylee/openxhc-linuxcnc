// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "openxhc/error.hpp"

#include <cstdint>
#include <optional>

namespace openxhc {
enum class DeviceState {
  Disconnected,
  Identified,
  Negotiating,
  ReadyInhibited,
  Armed,
  Streaming,
  Holding,
  Faulted
};

class StateMachine {
 public:
  DeviceState current() const noexcept;
  Status transition(DeviceState next);
  void fault(Error error);
  Status acknowledge_fault(bool communication_healthy, bool outputs_inhibited);
  const Error* first_fault() const noexcept;
  std::uint64_t fault_count() const noexcept;

 private:
  DeviceState current_{DeviceState::Disconnected};
  std::optional<Error> first_fault_;
  std::uint64_t fault_count_{};
};
}  // namespace openxhc
