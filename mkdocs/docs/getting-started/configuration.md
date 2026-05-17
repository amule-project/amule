# Configuration

This page covers the initial configuration steps you should complete before using aMule for the first time.

## Port Setup

To achieve a **HighID** (full connectivity) on the eD2k network, the following ports must be open and forwarded in your router or firewall:

| Port | Protocol | Purpose |
|---|---|---|
| 4662 | TCP | eD2k client connections |
| 4665 | UDP | eD2k server communication |
| 4672 | UDP | Kademlia network |

Alternatively, enable **UPnP** in Preferences to let aMule configure your router automatically (requires a UPnP-capable router and the `libupnp` dependency).

## Bandwidth Limits

Set your upload and download limits to approximately **80% of your actual line speed** (in kB/s, not kbps). Leaving some headroom prevents aMule from saturating your connection and keeps latency low for other traffic.

## Shared Folders

Choose shared folders carefully. Never share sensitive system directories (e.g., `/home`, `C:\Users`). Create a dedicated download/share folder instead.

## Running amuled (Headless)

To run aMule as a background daemon:

1. Run `amuled` once to generate the default configuration file.
2. Set an **EC password** in `~/.aMule/amule.conf` under `[ExternalConnect]`:
   ```ini
   [ExternalConnect]
   AcceptExternalConnections=1
   ECPassword=<md5 hash of your password>
   ```
3. Restart `amuled`. Connect to it with `amulegui`, `amuleweb`, or `amulecmd`.
