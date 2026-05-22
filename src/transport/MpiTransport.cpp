// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// MPI implementation of the Transport interface.

#include "mqss/transport/MpiTransport.hpp"

#include "mqss/Message.hpp"
#include "mqss/Status.hpp"

#include <mpi.h>

#include <atomic>
#include <charconv>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace mqss {
namespace {

// MpiTransport is an MPI-based Transport implementation using MPI
// point-to-point communication.
//
// Address model:
// - Address.name is interpreted as a decimal MPI rank within the configured
//   communicator.
// - send() sends to the destination rank.
// - receive() receives from the source rank.
// - One configured MPI tag is used per transport instance.
//
// MPI lifetime model:
// - By default, MPI is expected to be initialized by the application.
// - If TransportOptions<Mpi>::initialize_mpi is true, the transport initializes
//   MPI when needed.
// - MPI is finalized by this transport only if this transport initialized it.
//
// Communicator model:
// - The configured communicator is duplicated during setup.
// - The transport owns and frees only the duplicated communicator.
// - MPI_ERRORS_RETURN is installed on the duplicated communicator so MPI
//   failures can be converted into Status values.
//
// Receive model:
// - receive() uses MPI_Iprobe() instead of a blocking MPI_Recv().
// - This allows timeout handling and lets receive() observe local shutdown.
// - When a message is available, receive() uses MPI_Recv() to consume it.
//
// Current limitations:
// - Only Envelope::payload is transmitted.
// - No manual acknowledgment support.
// - No requeue support.
// - No send confirmation support.
// - No wildcard source receive support.
// - No explicit flow control or backpressure.
class MpiTransport final : public Transport {
public:
  explicit MpiTransport(const TransportOptions<Mpi> &options)
      : tag_(options.tag) {
    setup(options);
  }

  ~MpiTransport() override { shutdown(); }

  FeatureSet features() const override {
    // No optional Transport features are currently supported.
    return FeatureSet{};
  }

  Status send(const Address &dest, const Envelope &envelope,
              const SendArgs &args = {}) override {
    if (!setup_status_.ok()) {
      return setup_status_;
    }

    if (args.confirm) {
      return Status::unsupported("send confirmation is not supported");
    }

    auto rank = parseRank(dest);
    if (!rank.ok()) {
      return rank.status;
    }

    auto rank_status = validateRank(rank.value, "destination");
    if (!rank_status.ok()) {
      return rank_status;
    }

    if (envelope.payload.size() > static_cast<std::size_t>(maxInt())) {
      return Status::invalidArgument("MPI payload is too large");
    }

    const auto payload_size = static_cast<int>(envelope.payload.size());

    int rc = MPI_Send(envelope.payload.data(), payload_size, MPI_CHAR,
                      rank.value, tag_, communicator_);
    if (rc != MPI_SUCCESS) {
      return mpiError("MPI_Send failed", rc);
    }

    return Status::success();
  }

  Result<Message> receive(const Address &src,
                          const ReceiveArgs &args = {}) override {
    if (!setup_status_.ok()) {
      return std::unexpected(setup_status_);
    }

    auto rank = parseRank(src);
    if (!rank.ok()) {
      return std::unexpected(rank.status);
    }

    auto rank_status = validateRank(rank.value, "source");
    if (!rank_status.ok()) {
      return std::unexpected(rank_status);
    }

    const auto start = std::chrono::steady_clock::now();

    while (!stopped_.load()) {
      int available = 0;
      MPI_Status probe_status{};

      // Poll rather than block so timeout and shutdown are observable.
      int rc = MPI_Iprobe(rank.value, tag_, communicator_, &available,
                          &probe_status);
      if (rc != MPI_SUCCESS) {
        return std::unexpected(mpiError("MPI_Iprobe failed", rc));
      }

      if (available) {
        return receiveAvailableMessage(rank.value, probe_status);
      }

      if (args.timeout.has_value()) {
        const auto timeout = *args.timeout;

        if (timeout.count() == 0) {
          return std::unexpected(Status::timeout("no message received"));
        }

        const auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
          return std::unexpected(Status::timeout("receive timed out"));
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return std::unexpected(Status::unavailable("transport shut down"));
  }

private:
  struct ParsedRank {
    int value = -1;
    Status status = Status::success();

    [[nodiscard]] bool ok() const { return status.ok(); }
  };

  void setup(const TransportOptions<Mpi> &options) {
    if (options.tag < 0) {
      setup_status_ = Status::invalidArgument("MPI tag must be non-negative");
      return;
    }

    int initialized = 0;
    int rc = MPI_Initialized(&initialized);
    if (rc != MPI_SUCCESS) {
      setup_status_ = mpiError("MPI_Initialized failed", rc);
      return;
    }

    if (!initialized) {
      if (!options.initialize_mpi) {
        setup_status_ = Status::unavailable("MPI is not initialized");
        return;
      }

      int provided = MPI_THREAD_SINGLE;
      rc = MPI_Init_thread(nullptr, nullptr, options.required_thread_level,
                           &provided);
      if (rc != MPI_SUCCESS) {
        setup_status_ = mpiError("MPI_Init_thread failed", rc);
        return;
      }

      owns_mpi_ = true;

      if (provided < options.required_thread_level) {
        setup_status_ =
            Status::unavailable("MPI implementation does not provide required "
                                "thread support level");
        return;
      }
    }

    // Own a duplicate communicator instead of mutating/freeing the caller's
    // communicator directly.
    rc = MPI_Comm_dup(options.communicator, &communicator_);
    if (rc != MPI_SUCCESS) {
      setup_status_ = mpiError("MPI_Comm_dup failed", rc);
      return;
    }

    owns_communicator_ = true;

    rc = MPI_Comm_set_errhandler(communicator_, MPI_ERRORS_RETURN);
    if (rc != MPI_SUCCESS) {
      setup_status_ = mpiError("MPI_Comm_set_errhandler failed", rc);
      return;
    }

    setup_status_ = Status::success();
  }

  void shutdown() {
    stopped_.store(true);

    if (owns_communicator_) {
      MPI_Comm_free(&communicator_);
      owns_communicator_ = false;
      communicator_ = MPI_COMM_NULL;
    }

    if (owns_mpi_) {
      int finalized = 0;
      MPI_Finalized(&finalized);

      if (!finalized) {
        MPI_Finalize();
      }

      owns_mpi_ = false;
    }
  }

  Result<Message> receiveAvailableMessage(int rank,
                                          const MPI_Status &probe_status) {
    int count = 0;
    int rc = MPI_Get_count(&probe_status, MPI_CHAR, &count);
    if (rc != MPI_SUCCESS) {
      return std::unexpected(mpiError("MPI_Get_count failed", rc));
    }

    if (count < 0) {
      return std::unexpected(
          Status::internal("MPI_Get_count returned invalid size"));
    }

    std::string payload(static_cast<std::size_t>(count), '\0');

    rc = MPI_Recv(payload.data(), count, MPI_CHAR, rank, tag_, communicator_,
                  MPI_STATUS_IGNORE);
    if (rc != MPI_SUCCESS) {
      return std::unexpected(mpiError("MPI_Recv failed", rc));
    }

    Message msg;
    msg.envelope.payload = std::move(payload);
    return msg;
  }

  Status validateRank(int rank, std::string_view role) const {
    int size = 0;
    int rc = MPI_Comm_size(communicator_, &size);
    if (rc != MPI_SUCCESS) {
      return mpiError("MPI_Comm_size failed", rc);
    }

    if (rank < 0 || rank >= size) {
      std::string reason = "MPI ";
      reason += role;
      reason += " rank is out of range";
      return Status::invalidArgument(std::move(reason));
    }

    return Status::success();
  }

  static ParsedRank parseRank(const Address &address) {
    if (address.name.empty()) {
      return {.status = Status::invalidArgument("MPI rank address is empty")};
    }

    int rank = -1;
    std::string_view text{address.name};

    const auto *begin = text.data();
    const auto *end = text.data() + text.size();

    auto [ptr, ec] = std::from_chars(begin, end, rank);

    if (ec != std::errc{} || ptr != end) {
      return {.status = Status::invalidArgument(
                  "MPI address must be a decimal rank")};
    }

    return {.value = rank};
  }

  Status mpiError(std::string prefix, int error_code) const {
    char buffer[MPI_MAX_ERROR_STRING]{};
    int length = 0;

    MPI_Error_string(error_code, buffer, &length);

    if (length > 0) {
      prefix += ": ";
      prefix.append(buffer, static_cast<std::size_t>(length));
    }

    return Status::unavailable(std::move(prefix));
  }

  static constexpr int maxInt() { return 2147483647; }

  MPI_Comm communicator_ = MPI_COMM_NULL;
  int tag_ = 0;

  bool owns_communicator_ = false;
  bool owns_mpi_ = false;

  std::atomic<bool> stopped_{false};

  Status setup_status_ = Status::unavailable("MPI transport not initialized");
};

} // namespace

template <>
std::unique_ptr<Transport>
createTransport<Mpi>(const TransportOptions<Mpi> &options) {
  return std::make_unique<MpiTransport>(options);
}

} // namespace mqss
