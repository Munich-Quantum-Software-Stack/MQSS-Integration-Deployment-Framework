// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Status and result types.

#pragma once

#include <expected>
#include <string>
#include <utility>

namespace mqss {

/// Status codes reported by operations.
enum class StatusCode {
  Ok = 0,
  Timeout,
  Unavailable,
  Unsupported,
  InvalidArgument,
  Serialization,
  Internal,
};

/// Status of an operation that does not return a value.
class [[nodiscard]] Status {
public:
  /// Success status.
  /// For success status, the reason is always empty.
  static Status success() { return Status(StatusCode::Ok, {}); }

  /// Timeout status.
  static Status timeout(std::string reason = {}) {
    return Status(StatusCode::Timeout, std::move(reason));
  }

  /// Unavailable status.
  static Status unavailable(std::string reason = {}) {
    return Status(StatusCode::Unavailable, std::move(reason));
  }

  /// Unsupported-operation status.
  static Status unsupported(std::string reason = {}) {
    return Status(StatusCode::Unsupported, std::move(reason));
  }

  /// Invalid-argument status.
  static Status invalidArgument(std::string reason = {}) {
    return Status(StatusCode::InvalidArgument, std::move(reason));
  }

  /// Serialization status.
  static Status serialization(std::string reason = {}) {
    return Status(StatusCode::Serialization, std::move(reason));
  }

  /// Internal-error status.
  static Status internal(std::string reason = {}) {
    return Status(StatusCode::Internal, std::move(reason));
  }

  /// Return true if this status represents success.
  [[nodiscard]] constexpr bool ok() const { return code_ == StatusCode::Ok; }

  /// Return the status code.
  [[nodiscard]] constexpr StatusCode code() const { return code_; }

  /// Return the explanatory reason.
  [[nodiscard]] const std::string &reason() const { return reason_; }

private:
  Status(StatusCode code, std::string reason)
      : code_(code), reason_(std::move(reason)) {}

  StatusCode code_;
  std::string reason_;
};

/// Result of an operation that either returns a value of type `T`
/// or a failure `Status`.
template <class T>
using Result = std::expected<T, Status>;

} // namespace mqss
