// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Transport-independent messaging model.

#pragma once

#include "mqss/Status.hpp"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace mqss {

/// Transport endpoint identifier.
///
/// `Address` identifies an endpoint within an already selected transport.
/// Its meaning is transport-specific, for example a queue name, topic,
/// service endpoint, or MPI rank-based endpoint.
struct Address {
  /// Transport-specific endpoint identifier.
  std::string name;
};

/// Transport-independent encoded message container.
///
/// `Envelope` carries a serialized payload together with optional metadata
/// used by codecs and messaging patterns such as request/reply.
struct Envelope {
  using Headers = std::unordered_map<std::string, std::string>;

  /// Serialized message payload.
  ///
  /// The payload is transport-opaque at this level and may contain binary or
  /// textual encoded data.
  std::string payload;

  /// Declared payload format or media type, e.g. "application/json".
  std::string content_type{};

  /// Additional application- or transport-specific message metadata.
  Headers headers{};

  /// Correlation identifier used to associate related messages, for example
  /// when matching requests and replies.
  std::string correlation_id{};

  /// Reply destination for request/reply patterns.
  std::optional<Address> reply_to{};
};

/// Delivered message object.
///
/// `Message` wraps a delivered `Envelope` and, when supported, transport-owned
/// completion hooks such as `ack()` and `nack()`.
class Message {
public:
  /// Delivered envelope.
  Envelope envelope;

  /// Acknowledge successful processing of this message.
  ///
  /// \return
  /// - `Status::success()` if the acknowledgment was applied,
  /// - `Status::unsupported(...)` if acknowledgment is not supported for this
  ///   message or transport mode,
  /// - a non-success status on failure.
  [[nodiscard]] Status ack() {
    if (!ack_impl_) {
      return Status::unsupported("ack not supported");
    }
    return ack_impl_();
  }

  /// Negative-acknowledge this message.
  ///
  /// \param requeue If true, request that the message be made available again
  ///        when supported by the transport.
  ///
  /// \return
  /// - `Status::success()` if the negative acknowledgment was applied,
  /// - `Status::unsupported(...)` if nack is not supported for this message or
  ///   transport mode,
  /// - a non-success status on failure.
  [[nodiscard]] Status nack(bool requeue = true) {
    if (!nack_impl_) {
      return Status::unsupported("nack not supported");
    }
    return nack_impl_(requeue);
  }

private:
  /// Passkey used to restrict ack/nack handler installation to transports.
  struct Passkey {
    friend class Transport;
    Passkey() = default;
  };

  using AckFn = std::function<Status()>;
  using NackFn = std::function<Status(bool)>;

  void setAckHandlers(Passkey, AckFn ack, NackFn nack) {
    ack_impl_ = std::move(ack);
    nack_impl_ = std::move(nack);
  }

  AckFn ack_impl_{};
  NackFn nack_impl_{};

  friend class Transport;
};

} // namespace mqss
