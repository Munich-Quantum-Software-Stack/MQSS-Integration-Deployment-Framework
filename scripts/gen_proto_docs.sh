#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

# Generate Markdown documentation from protobuf definitions.

# Resolve project root (assumes script is in a first-level subdirectory).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

PROTO_DIR="$ROOT_DIR/protocol/proto"
DOCS_DIR="$ROOT_DIR/docs"
PROTO_FILE="$PROTO_DIR/v1/messages.proto"
TEMPLATE_FILE="$PROTO_DIR/templates/markdown.tmpl"

PROTOC_GEN_DOC="${PROTOC_GEN_DOC:-$(command -v protoc-gen-doc || true)}"

if [[ -z "$PROTOC_GEN_DOC" ]]; then
  echo "Error: protoc-gen-doc not found in PATH" >&2
  echo "Install it or set PROTOC_GEN_DOC=/path/to/protoc-gen-doc" >&2
  exit 1
fi

echo "Generating Markdown documentation from messages.proto..."

protoc \
  --proto_path="$PROTO_DIR" \
  --plugin="protoc-gen-doc=$PROTOC_GEN_DOC" \
  --doc_out="$DOCS_DIR" \
  --doc_opt="$TEMPLATE_FILE,messages.md" \
  "$PROTO_FILE"
