# amulegui (Remote GUI)

`amulegui` is a standalone graphical client that connects to a running `amuled` instance over the **External Connections (EC)** protocol. It provides the same GUI experience as `amule` but operates remotely.

## When to Use

Use `amulegui` when `amuled` is running on a different machine (or headlessly on the same machine) and you want a **native GUI** to control it.

## Connecting

1. Ensure `amuled` is running and `AcceptExternalConnections=1` is set in its configuration.
2. Launch `amulegui`.
3. In the connection dialog, enter:
   - **Host**: hostname or IP address of the machine running `amuled`
   - **Port**: EC port (default `4712`)
   - **Password**: the EC password configured in `amuled`

## Features

`amulegui` exposes the same tabs as the local `amule` GUI:

- Transfers, Search, Shared Files, Statistics, Kad, Servers, Messages.

## Network Requirements

The EC port (default 4712) must be reachable from the machine running `amulegui`. If connecting over the internet, consider tunneling through SSH.

## See Also

- [EC Protocol](../advanced/ec-protocol.md) — the binary protocol underlying the connection.
