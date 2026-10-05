<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)"
            srcset="docs/assets/mqss_logo_dark.svg">
    <img src="docs/assets/mqss_logo.svg"
         width="20%"
         alt="MQSS logo">
  </picture>
</p>

# MQSS Integration Deployment Framework

<p align="center">
  <a href="https://munich-quantum-software-stack.github.io/MQSS-Integration-Deployment-Framework/">
  <img style="min-width: 200px !important; width: 30%;" src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA0NDggNTEyIj48IS0tIUZvbnQgQXdlc29tZSBGcmVlIDYuNi4wIGJ5IEBmb250YXdlc29tZSAtIGh0dHBzOi8vZm9udGF3ZXNvbWUuY29tIExpY2Vuc2UgLSBodHRwczovL2ZvbnRhd2Vzb21lLmNvbS9saWNlbnNlL2ZyZWUgQ29weXJpZ2h0IDIwMjQgRm9udGljb25zLCBJbmMuLS0+PHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTk2IDBDNDMgMCAwIDQzIDAgOTZMMCA0MTZjMCA1MyA0MyA5NiA5NiA5NmwyODggMCAzMiAwYzE3LjcgMCAzMi0xNC4zIDMyLTMycy0xNC4zLTMyLTMyLTMybDAtNjRjMTcuNyAwIDMyLTE0LjMgMzItMzJsMC0zMjBjMC0xNy43LTE0LjMtMzItMzItMzJMMzg0IDAgOTYgMHptMCAzODRsMjU2IDAgMCA2NEw5NiA0NDhjLTE3LjcgMC0zMi0xNC4zLTMyLTMyczE0LjMtMzIgMzItMzJ6bTMyLTI0MGMwLTguOCA3LjItMTYgMTYtMTZsMTkyIDBjOC44IDAgMTYgNy4yIDE2IDE2cy03LjIgMTYtMTYgMTZsLTE5MiAwYy04LjggMC0xNi03LjItMTYtMTZ6bTE2IDQ4bDE5MiAwYzguOCAwIDE2IDcuMiAxNiAxNnMtNy4yIDE2LTE2IDE2bC0xOTIgMGMtOC44IDAtMTYtNy4yLTE2LTE2czcuMi0xNiAxNi0xNnoiLz48L3N2Zz4=" alt="Documentation" />
  </a>
</p>

The framework provides an integration layer for communication between software components in a distributed quantum system running in an HPC environment.

Components can interact either through a generic messaging transport or through direct interface calls when deployed within a single process.
This enables flexible deployment and execution across different runtime configurations.

## Design Concepts

The framework separates application logic from the underlying communication mechanism.
It is structured around three main concepts:

- **Transport abstraction**
  Provides a transport-neutral messaging interface that can be implemented using different communication backends.
  It relies on a language-agnostic protocol defining message structure and serialization.

- **Service / domain interfaces**
  Define component functionality independently of the communication mechanism.

- **Transport adapters**
  Bridge service interfaces to the transport layer, allowing services to be exposed or consumed via messaging.

## Version Management

The framework supports component versioning to ensure consistent communication between components and to allow deployment of specific versions when required.

## Deployment Options

The framework allows flexible deployment without changes to application logic:

- **Distributed nodes**
  Components run on separate nodes and communicate via a messaging backend.

- **Local composition**
  Multiple components run within a single process and communicate through direct interface calls.

- **Hybrid systems**
  A combination of both approaches, where some components communicate locally while others use a messaging backend.
