// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Typed messaging facade built on `Transport` and `Codec`.

#pragma once

#include "mqss/Message.hpp"
#include "mqss/Status.hpp"
#include "mqss/protocol/Codec.hpp"
#include "mqss/transport/Transport.hpp"

#include <memory>
#include <utility>

namespace mqss {

/// Messenger combines a transport backend with a codec format.
///
/// `Backend` selects the transport implementation.
/// `Format` selects the encoding format used by `Codec<T, Format>`.
///
/// This class provides typed messaging on top of the untyped transport API:
///
/// - `send<T>()` encodes and sends a typed message
/// - `receive<T>()` receives and decodes one typed message
template <class Backend, class Format>
class Messenger {
public:
  using Options = TransportOptions<Backend>;

  /// Construct a messenger using default backend options.
  Messenger() : Messenger(Options{}) {}

  /// Construct a messenger using explicit backend options.
  explicit Messenger(const Options &options)
      : transport_(createTransport<Backend>(options)) {}

  Messenger(const Messenger &) = delete;
  Messenger &operator=(const Messenger &) = delete;

  Messenger(Messenger &&) noexcept = default;
  Messenger &operator=(Messenger &&) noexcept = default;

  ~Messenger() = default;

  /// Encode and send a typed message.
  ///
  /// This is the common send operation for typed messaging. The value is
  /// encoded using `Codec<T, Format>` and then passed to the underlying
  /// transport.
  ///
  /// \param dest Destination address.
  /// \param value Typed payload to encode and send.
  /// \param args Optional send parameters forwarded to `Transport::send()`.
  ///
  /// \return
  /// - `Status::success()` if the message was accepted by the transport,
  /// - a serialization error if encoding fails,
  /// - a transport error if sending fails.
  template <class T>
  Status send(const Address &dest, const T &value, const SendArgs &args = {}) {
    auto encoded = Codec<T, Format>::encode(value);
    if (!encoded) {
      return encoded.error();
    }

    return transport_->send(dest, *encoded, args);
  }

  /// Receive and decode one typed message.
  ///
  /// This is the common receive operation for callers that only need the typed
  /// payload. The underlying transport message is consumed internally.
  ///
  /// \param src Source address to receive from.
  /// \param args Optional receive parameters.
  ///
  /// \return The decoded typed payload, or a transport/decode error.
  template <class T>
  Result<T> receive(const Address &src, const ReceiveArgs &args = {}) {
    auto received = receiveExtended<T>(src, args);
    if (!received) {
      return std::unexpected(received.error());
    }

    return std::move(received->first);
  }

  /// Receive and decode one typed message together with the original transport
  /// message.
  ///
  /// This variant is intended for callers that need access to transport-level
  /// details such as metadata or acknowledgment operations.
  ///
  /// \param src Source address to receive from.
  /// \param args Optional receive parameters.
  ///
  /// \return A pair containing the decoded typed payload and the original
  /// transport message, or a transport/decode error.
  template <class T>
  Result<std::pair<T, Message>> receiveExtended(const Address &src,
                                                const ReceiveArgs &args = {}) {
    auto received = transport_->receive(src, args);
    if (!received) {
      return std::unexpected(received.error());
    }

    Message msg = std::move(*received);

    auto decoded = Codec<T, Format>::decode(msg.envelope);
    if (!decoded) {
      return std::unexpected(decoded.error());
    }

    return std::make_pair(std::move(*decoded), std::move(msg));
  }

  /// Access the underlying transport features.
  [[nodiscard]] FeatureSet features() const { return transport_->features(); }

private:
  std::unique_ptr<Transport> transport_;
};

} // namespace mqss
