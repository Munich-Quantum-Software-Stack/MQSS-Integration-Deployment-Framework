// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// In-memory implementation of the Transport interface.

#include "mqss/transport/InMemoryTransport.hpp"
#include "mqss/Message.hpp"
#include "mqss/Status.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace mqss {

namespace {

// This implementation is primarily intended for testing and early design
// validation. It provides a simple, deterministic transport using
// in-memory queues.
//
// Design notes:
// - send() enqueues messages into per-address FIFO queues
// - receive() performs synchronous, pull-based retrieval
// - no background threads or async mechanisms are used internally
//
// Behavioral properties:
// - messages are delivered in FIFO order per address
// - receive() blocks according to ReceiveArgs::timeout
// - shutdown unblocks all waiting receivers with `unavailable`
//
// Limitations:
// - no persistence
// - no delivery guarantees beyond in-memory storage
// - no support for optional features (ack, requeue, confirm)
class InMemoryTransport final : public Transport {
public:
  InMemoryTransport() : state_(std::make_shared<State>()) {}
  ~InMemoryTransport() override { shutdown(); }

  FeatureSet features() const override {
    // No optional features supported.
    return FeatureSet{};
  }

  /// Send one message by enqueueing it into the destination queue.
  Status send(const Address &dest, const Envelope &envelope,
              const SendArgs &args = {}) override {
    if (dest.name.empty()) {
      return Status::invalidArgument("send destination name is empty");
    }

    // Send confirmation is not supported in this implementation.
    if (args.confirm) {
      return Status::unsupported("send confirmation is not supported");
    }

    auto state = state_;
    if (!state) {
      return Status::unavailable("transport not initialized");
    }

    {
      std::lock_guard<std::mutex> lock(state->mu);
      if (state->stopped) {
        return Status::unavailable("transport shut down");
      }

      // Enqueue message for later receive().
      state->queues[dest.name].push_back(envelope);
    }

    // Wake up any waiting receivers.
    state->cv.notify_all();
    return Status::success();
  }

  /// Receive one message from the source queue.
  ///
  /// This is a synchronous (blocking) operation depending on timeout policy.
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

    std::unique_lock<std::mutex> lock(state->mu);

    // Condition: message available OR transport stopped
    auto ready = [&] {
      if (state->stopped) {
        return true;
      }

      auto it = state->queues.find(src.name);
      return it != state->queues.end() && !it->second.empty();
    };

    bool waited_with_timeout = false;

    // Wait according to timeout policy.
    if (!ready()) {
      if (args.timeout.has_value()) {
        const auto timeout = *args.timeout;

        if (timeout.count() == 0) {
          return std::unexpected(Status::timeout("no message received"));
        }

        waited_with_timeout = true;
        state->cv.wait_for(lock, timeout, ready);
      } else {
        state->cv.wait(lock, ready);
      }
    }

    // Transport stopped while waiting.
    if (state->stopped) {
      return std::unexpected(Status::unavailable("transport shut down"));
    }

    // No message available after wait.
    auto it = state->queues.find(src.name);
    if (it == state->queues.end() || it->second.empty()) {
      return std::unexpected(waited_with_timeout
                                 ? Status::timeout("receive timed out")
                                 : Status::timeout("no message received"));
    }

    Envelope envelope = std::move(it->second.front());
    it->second.pop_front();

    if (it->second.empty()) {
      state->queues.erase(it);
    }

    return Result<Message>{makeMessageFromEnvelope(std::move(envelope))};
  }

private:
  /// Shared state across all operations.
  ///
  /// Stored in a shared_ptr so that in-flight receive() calls remain valid
  /// even if the Transport object is destroyed.
  struct State {
    std::mutex mu;
    std::condition_variable cv;
    bool stopped = false;

    // Per-address FIFO queues.
    std::unordered_map<std::string, std::deque<Envelope>> queues;
  };

  /// Shutdown the transport and wake all waiting receivers.
  void shutdown() {
    auto state = state_;
    if (!state) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(state->mu);
      if (state->stopped) {
        return;
      }

      state->stopped = true;
    }

    state->cv.notify_all();
  }

  /// Convert Envelope to Message (no ack handlers in this transport).
  static Message makeMessageFromEnvelope(Envelope &&envelope) {
    Message msg;
    msg.envelope = std::move(envelope);
    return msg;
  }

  // Shared state used by all operations.
  // Kept in a shared_ptr so a blocking receive() on another thread can
  // see shutdown after the transport instance is destroyed.
  std::shared_ptr<State> state_;
};

} // namespace

template <>
std::unique_ptr<Transport>
createTransport<InMemory>(const TransportOptions<InMemory> &) {
  return std::make_unique<InMemoryTransport>();
}

} // namespace mqss
