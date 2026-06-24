# MQSS Consumer Example

This example demonstrates a minimal MQSS in-memory message roundtrip.

It creates a `QuantumTask`, sends it to an in-memory queue, receives it back, and verifies that the decoded message matches the original task.

The example uses:

- `mqss::InMemory` transport
- `mqss::ProtoBinary` protocol
- `mqss::Messenger`
- `mqss::QuantumTask`

## Build and Run

This example is built as an external consumer of an installed MQSS package.

See the project development guide for build, installation, and test instructions.
