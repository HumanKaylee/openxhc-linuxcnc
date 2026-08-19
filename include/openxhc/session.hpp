// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "openxhc/error.hpp"
#include "openxhc/state_machine.hpp"
#include "openxhc/status_report.hpp"
#include "openxhc/transport.hpp"

#include <chrono>
#include <cstdint>
#include <memory>

namespace openxhc {
// Drives the documented lifecycle over an injected transport.
//
// The read-only session stops at ReadyInhibited and never advances to Armed. Arming is
// the point at which a driver would accept motion, and nothing in this project has
// earned that: no output report has been captured, decoded, or bench verified. The
// transport cannot write either, so this is defence in depth rather than the only guard.
//
// Reconnect deliberately re-enters the lifecycle at Disconnected and walks forward again.
// Neither a reconnect nor a fault acknowledgement may restore authority to move.
class ReadOnlySession {
 public:
  explicit ReadOnlySession(std::unique_ptr<ITransport> transport) noexcept;

  // Disconnected -> Identified -> Negotiating -> ReadyInhibited.
  Status open();

  // Reads one record and accumulates it. Requires ReadyInhibited.
  Status poll(std::chrono::milliseconds timeout);

  // A lost link returns the lifecycle to Disconnected without faulting: on this
  // controller a periodic reset is the expected environment, not an error.
  Status handle_disconnect();

  DeviceState state() const noexcept { return machine_.current(); }
  const StateMachine& machine() const noexcept { return machine_; }
  const FieldActivity& activity() const noexcept { return activity_; }
  std::uint64_t records() const noexcept { return records_; }
  std::uint64_t malformed() const noexcept { return malformed_; }

  void close() noexcept;

 private:
  std::unique_ptr<ITransport> transport_;
  StateMachine machine_;
  FieldActivity activity_;
  std::uint64_t records_{};
  std::uint64_t malformed_{};
  bool closed_{};
};
}  // namespace openxhc
