// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// RabbitMQ-based Transport implementation built on top of SimpleAmqpClient.

#include "mqss/transport/RabbitMqSimpleTransport.hpp"

#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <atomic>
#include <chrono>
#include <expected>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

using namespace std::chrono_literals;

namespace mqss {
namespace {

// RabbitMqSimpleTransport is a RabbitMQ-based Transport implementation built
// on top of SimpleAmqpClient.
//
// Concurrency model:
// - SimpleAmqpClient provides a blocking API.
// - send() uses one shared publishing channel protected by a mutex.
// - receive() opens a separate channel for each call.
// - Channels are not used concurrently from multiple threads.
//
// Queue model:
// - Address.name is treated as a RabbitMQ queue name.
// - send() publishes to the default exchange ("") using the queue name as the
//   routing key.
// - Queues are declared on demand as regular queues.
// - Queue declaration is intentionally simple and may be refined later.
//
// Error handling:
// - SimpleAmqpClient uses exceptions for error reporting.
// - All exceptions are caught at the transport boundary and converted into
//   Status values.
//
// Current limitations:
// - No send confirmation support.
// - No reconnection or recovery logic.
// - No explicit rate control or backpressure.
// - Queues are declared with fixed defaults: non-durable, non-exclusive,
//   non-auto-delete.
class RabbitMqSimpleTransport final : public Transport {
public:
  explicit RabbitMqSimpleTransport(TransportOptions<RabbitMqSimple> opts)
      : state_(std::make_shared<State>()) {
    state_->opts = std::move(opts);
  }

  ~RabbitMqSimpleTransport() override { shutdown(); }

  FeatureSet features() const override {
    return FeatureSet{
        Feature::ManualAck,
        Feature::Requeue,
    };
  }

  Status send(const Address &dest, const Envelope &envelope,
              const SendArgs &args = {}) override {
    if (dest.name.empty()) {
      return Status::invalidArgument("send destination name is empty");
    }

    if (args.confirm) {
      return Status::unsupported("send confirmation is not supported");
    }

    auto state = state_;
    if (!state) {
      return Status::unavailable("transport not initialized");
    }

    if (state->stopped.load()) {
      return Status::unavailable("transport shut down");
    }

    try {
      auto bm = toBasicMessage(envelope);

      std::lock_guard<std::mutex> lock(state->send_mu);

      if (state->stopped.load()) {
        return Status::unavailable("transport shut down");
      }

      // Make channel creation lazy. If the constructor does it, it can throw
      // before any `Status` can be returned.
      if (!state->send_ch) {
        state->send_ch = openChannel(state->opts);
      }

      // Declare destination queue on demand.
      state->send_ch->DeclareQueue(dest.name, false, false, false, false);
      state->send_ch->BasicPublish("", dest.name, bm);

      return Status::success();
    } catch (const std::exception &e) {
      return Status::unavailable(std::string("send failed: ") + e.what());
    }
  }

  Result<Message> receive(const Address &src,
                          const ReceiveArgs &args = {}) override {
    if (src.name.empty()) {
      return std::unexpected(
          Status::invalidArgument("receive source name is empty"));
    }

    auto state = state_;
    if (!state) {
      return std::unexpected(Status::unavailable("transport not initialized"));
    }

    if (state->stopped.load()) {
      return std::unexpected(Status::unavailable("transport shut down"));
    }

    try {
      auto ch = openChannel(state->opts);

      // Declare source queue on demand.
      ch->DeclareQueue(src.name, false, false, false, false);

      DeliveryPtr delivery;

      // Non-blocking poll.
      if (args.timeout && *args.timeout == std::chrono::milliseconds{0}) {
        const bool ok =
            ch->BasicGet(delivery, src.name, args.ack_mode == AckMode::Auto);

        if (!ok) {
          return std::unexpected(Status::timeout("no message received"));
        }

        return makeMessageFromDelivery(ch, delivery, args.ack_mode);
      }

      const std::string consumer_tag = ch->BasicConsume(
          src.name, "", true, args.ack_mode == AckMode::Auto, false, 1);

      const auto cancel_consumer = [&] {
        try {
          ch->BasicCancel(consumer_tag);
        } catch (...) {
          // Ignore cleanup errors.
        }
      };

      // Wait indefinitely, but use bounded polling internally so shutdown can
      // be observed without requiring cross-thread channel cancellation.
      if (!args.timeout.has_value()) {
        while (!state->stopped.load()) {
          const bool ok = ch->BasicConsumeMessage(consumer_tag, delivery, 200);
          if (ok) {
            cancel_consumer();
            return makeMessageFromDelivery(ch, delivery, args.ack_mode);
          }
        }

        cancel_consumer();
        return std::unexpected(Status::unavailable("transport shut down"));
      }

      // Wait up to the specified timeout.
      const auto deadline = std::chrono::steady_clock::now() + *args.timeout;

      while (!state->stopped.load()) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
          cancel_consumer();
          return std::unexpected(Status::timeout("receive timed out"));
        }

        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline -
                                                                  now);
        const auto wait_ms =
            static_cast<int>(std::min(remaining, 200ms).count());

        const bool ok =
            ch->BasicConsumeMessage(consumer_tag, delivery, wait_ms);

        if (ok) {
          cancel_consumer();
          return makeMessageFromDelivery(ch, delivery, args.ack_mode);
        }
      }

      cancel_consumer();
      return std::unexpected(Status::unavailable("transport shut down"));
    } catch (const std::exception &e) {
      return std::unexpected(
          Status::unavailable(std::string("receive failed: ") + e.what()));
    }
  }

private:
  using ChannelPtr = AmqpClient::Channel::ptr_t;
  using BasicMessagePtr = AmqpClient::BasicMessage::ptr_t;
  using DeliveryPtr = AmqpClient::Envelope::ptr_t;

  // Open a new RabbitMQ channel using the configured transport options.
  static ChannelPtr openChannel(const TransportOptions<RabbitMqSimple> &opts) {
    return AmqpClient::Channel::Create(opts.host, opts.port, opts.username,
                                       opts.password, opts.vhost);
  }

  // Per-message manual-ack state. Keeps the delivery and channel alive and
  // ensures ack/nack is applied only once.
  struct AckState {
    std::mutex mu;
    bool completed = false;
    ChannelPtr ch;
    DeliveryPtr delivery;
  };

  struct State {
    TransportOptions<RabbitMqSimple> opts;
    std::atomic<bool> stopped{false};

    // Shared send channel.
    // Lazily opened by send() and used under send_mu.
    ChannelPtr send_ch;
    std::mutex send_mu;
  };

  // Build a Message from a RabbitMQ delivery.
  static Message makeMessageFromDelivery(ChannelPtr ch,
                                         const DeliveryPtr &delivery,
                                         AckMode ack_mode) {
    Message msg;
    msg.envelope = fromBasicMessage(*delivery->Message());

    if (ack_mode != AckMode::Manual) {
      return msg;
    }

    auto state = std::make_shared<AckState>();
    state->ch = std::move(ch);
    state->delivery = delivery;

    setAckHandlers(
        msg,
        [state]() -> Status {
          std::lock_guard<std::mutex> lock(state->mu);
          if (state->completed) {
            return Status::internal("message already acked/nacked");
          }

          try {
            state->ch->BasicAck(state->delivery);
            state->completed = true;
            return Status::success();
          } catch (const std::exception &e) {
            return Status::internal(std::string("ack failed: ") + e.what());
          }
        },
        [state](bool requeue) -> Status {
          std::lock_guard<std::mutex> lock(state->mu);
          if (state->completed) {
            return Status::internal("message already acked/nacked");
          }

          try {
            state->ch->BasicReject(state->delivery, requeue);
            state->completed = true;
            return Status::success();
          } catch (const std::exception &e) {
            return Status::internal(std::string("nack failed: ") + e.what());
          }
        });

    return msg;
  }

  // Convert a generic transport envelope into a RabbitMQ BasicMessage.
  static BasicMessagePtr toBasicMessage(const Envelope &envelope) {
    auto msg = AmqpClient::BasicMessage::Create(envelope.payload);

    if (!envelope.content_type.empty()) {
      msg->ContentType(envelope.content_type);
    }

    if (!envelope.correlation_id.empty()) {
      msg->CorrelationId(envelope.correlation_id);
    }

    if (envelope.reply_to.has_value() && !envelope.reply_to->name.empty()) {
      msg->ReplyTo(envelope.reply_to->name);
    }

    if (!envelope.headers.empty()) {
      AmqpClient::Table table;
      for (const auto &[k, v] : envelope.headers) {
        table[k] = AmqpClient::TableValue(v);
      }
      msg->HeaderTable(table);
    }

    return msg;
  }

  // Convert a RabbitMQ BasicMessage into a generic transport envelope.
  static Envelope fromBasicMessage(const AmqpClient::BasicMessage &msg) {
    Envelope envelope;
    envelope.payload = msg.Body();

    if (msg.ContentTypeIsSet()) {
      envelope.content_type = msg.ContentType();
    }

    if (msg.CorrelationIdIsSet()) {
      envelope.correlation_id = msg.CorrelationId();
    }

    if (msg.ReplyToIsSet()) {
      envelope.reply_to = Address{msg.ReplyTo()};
    }

    if (msg.HeaderTableIsSet()) {
      for (const auto &[k, v] : msg.HeaderTable()) {
        try {
          envelope.headers.emplace(k, v.GetString());
        } catch (...) {
          // Ignore non-string headers.
        }
      }
    }

    return envelope;
  }

  // Shut down the transport. Blocking receive() calls observe this through
  // bounded polling and return Status::unavailable(...).
  void shutdown() {
    auto state = state_;
    if (!state) {
      return;
    }

    bool expected = false;
    state->stopped.compare_exchange_strong(expected, true);
  }

  std::shared_ptr<State> state_;
};

} // namespace

template <>
std::unique_ptr<Transport> createTransport<RabbitMqSimple>(
    const TransportOptions<RabbitMqSimple> &options) {
  return std::make_unique<RabbitMqSimpleTransport>(options);
}

} // namespace mqss
