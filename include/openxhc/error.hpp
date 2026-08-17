// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace openxhc {
enum class ErrorCode { InvalidArgument, UnsupportedDevice, InvalidReport, InvalidTransition,
                       Timeout, Disconnected, WriteDisabled, ParseError };
struct Error { ErrorCode code; std::string message; };
template <class T> using Result = std::variant<T, Error>;
struct Status {
  bool success;
  std::optional<Error> error;
  static Status ok() { return {true, std::nullopt}; }
  static Status fail(ErrorCode code, std::string message) {
    return {false, Error{code, std::move(message)}};
  }
};
}
