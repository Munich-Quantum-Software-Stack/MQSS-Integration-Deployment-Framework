// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Generic typed-message codec interface.

#pragma once

namespace mqss {

/// `Codec<T, Format>` converts between a typed message `T` and a transport
/// `Envelope` using the specified encoding format.
///
/// Specializations must provide:
///
///   static Result<Envelope> encode(const T &);
///   static Result<T> decode(const Envelope &);
template <class T, class Format>
struct Codec;

/// Format tag for the Protobuf binary wire format.
struct ProtoBinary {};

/// Format tag for the Protobuf JSON representation.
struct ProtoJson {};

} // namespace mqss
