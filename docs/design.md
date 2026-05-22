# Design Overview

This document describes the conceptual design of the integration layer.

The integration layer provides transport-independent messaging abstractions, typed message exchange, and backend-specific transport implementations behind a common interface.

The design focuses on:

- explicit error handling
- separation of concerns
- transport independence
- typed application messaging
- extensible transport backends
- predictable behavior

The API reference for concrete classes and functions is provided separately through the generated Doxygen documentation.

## Status and Result Model

The integration layer uses an explicit value-or-error model for reporting operation outcomes.

Operations do not throw exceptions. Instead, they return either:

- a status object for operations without a return value
- a result object containing either a value or an error status

Errors are represented independently of transport and protocol layers.

## Message Model

The integration layer defines a transport-independent message model used to exchange data between components.
Payload encoding is separated from message delivery, and optional transport features are exposed without forcing uniform semantics.

### Address

An address identifies a transport-specific endpoint.

The interpretation of an address depends on the transport backend.
Examples include queue names, topics, service identifiers, MPI ranks, or logical endpoints.

### Envelope

An envelope represents a transport-neutral encoded message.

It contains:

- a serialized payload
- message metadata
- optional routing or correlation information

The payload remains opaque at this layer and is interpreted by codecs.

### Message

A message represents a delivered transport message.

Depending on transport capabilities, a message may support transport-specific completion operations such as acknowledgment or rejection.

Optional features that are not supported by a transport are reported explicitly.

## Typed Messages and Encoding

Application code operates on typed messages rather than serialized payloads.

Typed messages are converted to and from transport envelopes using codecs.
Message formats can evolve independently from transports.

### Codec

A codec defines how a typed application message is encoded into a transport envelope and decoded back into a typed representation.

Encoding and decoding failures are reported using the common status model.

### Format

Encoding formats identify the representation used by a codec.

Examples include:

- binary Protocol Buffers
- JSON representations

Encoding format selection is independent of the transport backend.

### Protocol Types

Application message types are defined using Protocol Buffers and exposed through stable library-level abstractions.

## Transport Interface

The transport layer provides a generic abstraction for message delivery between components.
The transport interface intentionally remains minimal and focuses on basic delivery semantics.
Detailed behavior such as ordering guarantees, persistence, or broker-specific features is defined by individual transport implementations.

### Overview

The transport abstraction defines two core operations:

- sending messages to a destination
- receiving messages from a source

Higher-level messaging patterns are expected to be implemented on top of these primitives.

### Capabilities

Transport implementations may provide optional capabilities such as:

- manual acknowledgment
- message requeue
- send confirmation

Capabilities are transport-dependent and may be queried at runtime.

## Backend Selection

Transport backends are selected using type-level configuration.
Backend-specific configuration remains isolated.
New backends can be introduced without modifying existing interfaces.

Each backend defines its own configuration and capability model while exposing the common transport abstraction.

## In-Memory Transport

The in-memory transport provides a simple transport implementation for use within a single process.

It is primarily intended for testing, prototyping, and validation of higher layers without requiring external infrastructure.

The implementation prioritizes deterministic behavior and minimal complexity over production-oriented messaging features.
It serves as a reference implementation of the transport contract.

### Behavior

- Messages are delivered in FIFO order per address.
- Receive operations support blocking and bounded-wait semantics.
- Message delivery is performed entirely in-process.
- Shutdown unblocks pending receive operations.

### Capabilities

The in-memory transport does not provide advanced transport features such as:

- manual acknowledgment
- requeue semantics
- send confirmation

Unsupported operations are reported explicitly.

## RabbitMQ Transport

The RabbitMQ transport provides a transport implementation based on a RabbitMQ message broker.

It enables communication across process and system boundaries while preserving the common transport abstraction.

### Behavior

- Message delivery is managed by the broker.
- Queue semantics and ordering follow RabbitMQ behavior.
- Receive operations support blocking and bounded-wait semantics.
- Connections and channels are managed internally by the transport layer.

### Capabilities

The RabbitMQ transport supports broker-backed features such as:

- manual acknowledgment
- negative acknowledgment and requeue

Capability availability depends on broker behavior and transport configuration.

### Configuration

The transport is configured through backend-specific option structures that define connection parameters and transport settings.

## MPI Transport

The MPI transport provides a transport implementation based on MPI point-to-point communication.

It enables communication between distributed processes participating in the same MPI environment while preserving the common transport abstraction.

### Behavior

- Addresses are interpreted as MPI ranks within a configured communicator.
- Message delivery uses MPI point-to-point communication.
- Receive operations support blocking and bounded-wait semantics.
- The transport duplicates and manages its own MPI communicator instance.
- MPI initialization may be managed externally or optionally by the transport.

### Capabilities

The MPI transport currently provides basic message delivery functionality and does not support advanced messaging features.

### Configuration

The transport is configured through backend-specific option structures that define:

- the MPI communicator
- the MPI message tag
- MPI initialization behavior
- required MPI thread support level

## Messenger

The messenger provides a typed messaging interface built on top of the transport and codec layers.
The messenger separates typed application logic from transport-specific message delivery details.

It combines:

- a transport backend
- a message encoding format

### Overview

The messenger enables application code to exchange typed messages without interacting directly with transport envelopes or serialized payloads.

Encoding, decoding, and transport interaction are coordinated internally by the messenger layer.

### Behavior

- Typed messages are encoded before transport delivery.
- Received transport messages are decoded into typed messages.
- Transport and codec errors are propagated through the common status model.
