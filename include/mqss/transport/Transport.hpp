// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Abstract untyped messaging transport interface.

#pragma once

#include "mqss/Message.hpp"
#include "mqss/Status.hpp"

#include <chrono>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>

namespace mqss {

/// Optional transport capabilities.
enum class Feature {
  ManualAck,   ///< `Message::ack()` / `Message::nack()` may be meaningful.
  Requeue,     ///< `nack(requeue=true)` may request requeue semantics.
  SendConfirm, ///< `send(confirm=true)` may request confirmation support.
};

/// Runtime set of supported transport features.
class FeatureSet {
public:
  constexpr FeatureSet() = default;

  constexpr FeatureSet(std::initializer_list<Feature> list) {
    for (auto feature : list) {
      add(feature);
    }
  }

  constexpr void add(Feature feature) {
    bits_[static_cast<std::size_t>(feature)] = true;
  }

  [[nodiscard]] constexpr bool has(Feature feature) const {
    return bits_[static_cast<std::size_t>(feature)];
  }

private:
  static constexpr std::size_t num_features_ = 8;
  bool bits_[num_features_]{};
};

/// Delivery acknowledgment mode.
enum class AckMode {
  Auto,  ///< Delivery is completed automatically by the transport.
  Manual ///< Caller completes delivery explicitly.
};

/// Optional arguments for `Transport::send()`.
struct SendArgs {
  /// Request send confirmation, if supported.
  ///
  /// If `confirm == true` and the transport does not support
  /// `Feature::SendConfirm`, `send()` must return `Status::unsupported(...)`.
  ///
  /// The exact meaning of confirmation is transport-specific.
  bool confirm = false;
};

/// Optional arguments for `Transport::receive()`.
struct ReceiveArgs {
  /// Timeout policy for the receive operation.
  ///
  /// Semantics:
  /// - `std::nullopt`: wait indefinitely until a message becomes available
  ///   or the operation otherwise completes.
  /// - `0ms`: non-blocking poll; return `Status::timeout(...)` if no message
  ///   is immediately available.
  /// - `>0ms`: wait up to the specified duration; return
  ///   `Status::timeout(...)` if no message becomes available in time.
  std::optional<std::chrono::milliseconds> timeout = std::nullopt;

  /// Acknowledgment mode for the delivered message.
  AckMode ack_mode = AckMode::Manual;
};

/// Abstract messaging transport interface.
///
/// `Transport` defines a generic messaging abstraction for:
/// - one-way send via `send()`,
/// - pull-based consumption via blocking `receive()`.
///
/// Higher-level messaging patterns, such as request/reply or subscriptions,
/// may be implemented above this interface using these primitives.
///
/// Thread-safety
/// Unless documented otherwise, implementations should support concurrent
/// calls to `send()` and `receive()` from multiple threads.
/// Any deviations should be documented explicitly.
///
/// Address model
/// `Address` is a transport-level endpoint identifier. Its interpretation is
/// transport-specific and must be documented by the implementation.
///
/// Capability model
/// Not all transports provide the same optional behavior. `features()` allows
/// callers to query runtime capabilities such as manual acknowledgment,
/// requeue support, or send confirmation.
class Transport {
public:
  virtual ~Transport() = default;

  /// Return the optional features supported by this transport instance.
  [[nodiscard]] virtual FeatureSet features() const = 0;

  /// Send one message to a destination.
  ///
  /// `send()` performs one-way message submission. It is intended for
  /// fire-and-forget communication where this operation itself does not
  /// provide a reply.
  ///
  /// \param dest Destination address.
  /// \param envelope Message envelope to send.
  /// \param args Optional send parameters.
  ///
  /// \return
  /// - `Status::success()` if the message was accepted according to the
  ///   guarantees documented by the implementation,
  /// - `Status::unsupported(...)` if `args.confirm == true` was requested but
  ///   send confirmation is not supported,
  /// - a non-success status on failure.
  ///
  /// A successful return means the transport accepted the message according to
  /// implementation-defined guarantees. It does not by itself imply end-to-end
  /// delivery, persistence, or broker-side confirmation unless explicitly
  /// documented or requested via a supported feature.
  [[nodiscard]] virtual Status send(const Address &dest,
                                    const Envelope &envelope,
                                    const SendArgs &args = {}) = 0;

  /// Pull one message from a source address.
  ///
  /// `receive()` provides pull-based consumption. Each call completes with
  /// either one message or a status describing why no message was obtained.
  ///
  /// \param src Source address to receive from.
  /// \param args Optional receive parameters such as timeout and
  ///        acknowledgment mode.
  ///
  /// \return
  /// - success: contains a delivered `Message`,
  /// - timeout or no message available: contains `Status::timeout(...)`,
  /// - unavailable transport, shutdown, or disconnect: contains
  ///   `Status::unavailable(...)`,
  /// - other failures: contain a status describing the error.
  ///
  /// Each call retrieves at most one message. The exact timeout behavior is
  /// controlled by `ReceiveArgs`; implementations should document any
  /// transport-specific constraints.
  [[nodiscard]] virtual Result<Message>
  receive(const Address &src, const ReceiveArgs &args = {}) = 0;

protected:
  /// Install transport-provided acknowledgment handlers into a `Message`.
  ///
  /// This helper allows transport implementations to attach `ack()` and
  /// `nack()` behavior while keeping mutation of `Message` restricted.
  ///
  /// \param message Message whose handlers will be set.
  /// \param ack_fn Function invoked by `Message::ack()`.
  /// \param nack_fn Function invoked by `Message::nack(bool requeue)`.
  static void setAckHandlers(Message &message, std::function<Status()> ack_fn,
                             std::function<Status(bool)> nack_fn) {
    message.setAckHandlers(Message::Passkey{}, std::move(ack_fn),
                           std::move(nack_fn));
  }
};

/// Backend selection and construction.
///
/// Transport backends are selected using type-level backend tags.
/// Each backend provides a corresponding `TransportOptions<Backend>`
/// specialization and a `createTransport<Backend>(...)` factory.
template <class TransportBackend>
struct TransportOptions;

template <class TransportBackend>
std::unique_ptr<Transport>
createTransport(const TransportOptions<TransportBackend> &options = {});

} // namespace mqss
