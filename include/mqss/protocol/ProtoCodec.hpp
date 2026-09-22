// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Protobuf codec specializations.

#pragma once

#include "mqss/Message.hpp"
#include "mqss/Status.hpp"
#include "mqss/protocol/Codec.hpp"

#include <concepts>
#include <expected>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <string>
#include <utility>

namespace mqss {

/// Constraint for Protobuf message types.
template <typename T>
concept ProtobufMessage = std::derived_from<T, google::protobuf::Message>;

/// Protobuf binary codec specialization.
template <ProtobufMessage T>
struct Codec<T, ProtoBinary> {
  /// Encode a Protobuf message using the binary wire format.
  static Result<Envelope> encode(const T &msg) {
    Envelope envelope;

    if (!msg.SerializeToString(&envelope.payload)) {
      return std::unexpected(
          Status::serialization("protobuf binary serialization failed"));
    }

    envelope.content_type = "application/x-protobuf";
    return envelope;
  }

  /// Decode a Protobuf message from the binary wire format.
  static Result<T> decode(const Envelope &envelope) {
    T msg;

    if (!msg.ParseFromString(envelope.payload)) {
      return std::unexpected(
          Status::serialization("protobuf binary parse failed"));
    }

    return msg;
  }
};

/// Protobuf JSON codec specialization.
template <ProtobufMessage T>
struct Codec<T, ProtoJson> {
  /// Encode a Protobuf message using the Protobuf JSON representation.
  static Result<Envelope> encode(const T &msg) {
    Envelope envelope;
    std::string json;

    google::protobuf::util::JsonPrintOptions options;
    options.preserve_proto_field_names = true;

    auto status =
        google::protobuf::util::MessageToJsonString(msg, &json, options);
    if (!status.ok()) {
      return std::unexpected(Status::serialization(
          "protobuf JSON serialization failed: " + status.message()));
    }

    envelope.payload = std::move(json);
    envelope.content_type = "application/json";
    return envelope;
  }

  /// Decode a Protobuf message from the Protobuf JSON representation.
  static Result<T> decode(const Envelope &envelope) {
    T msg;

    auto status =
        google::protobuf::util::JsonStringToMessage(envelope.payload, &msg);
    if (!status.ok()) {
      return std::unexpected(Status::serialization(
          "protobuf JSON parse failed: " + status.message()));
    }

    return msg;
  }
};

} // namespace mqss
