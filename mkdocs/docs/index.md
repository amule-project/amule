# aMule Documentation

aMule is a free, open-source, multi-platform peer-to-peer file-sharing client for the **eD2k (eDonkey2000)** and **Kademlia (Kad)** networks. It is written in C++ using wxWidgets and runs on Linux, FreeBSD, OpenBSD, macOS, and Windows.

Originally a fork of xMule (itself a fork of lMule and eMule), aMule has been actively maintained since 2003.

## Binaries

| Binary | Description |
|---|---|
| `amule` | All-in-one GUI client |
| `amuled` | Headless daemon (no GUI) |
| `amulegui` | Remote GUI; connects to `amuled` via the EC protocol |
| `amuleweb` | HTTP web interface for a running `amuled` |
| `amulecmd` | Interactive command-line interface for a running `amuled` |

## Quick Start

- [Installation](getting-started/installation.md) — build dependencies and compile instructions
- [Configuration](getting-started/configuration.md) — initial setup and port configuration
- [Interface Overview](user-guide/interface.md) — a tour of the main GUI windows

## Supported Platforms

Linux, FreeBSD, OpenBSD, macOS, and Windows (x86\_64 and ARM64).
