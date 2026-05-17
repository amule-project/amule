# aMule

![aMule](https://raw.githubusercontent.com/amule-org/amule/master/org.amule.aMule.png)

aMule is an eMule-like client for the eDonkey and Kademlia networks.

[Forum] | [Wiki] | [FAQ]

[![Debian CI](https://badges.debian.net/badges/debian/stable/amule/version.svg)](https://buildd.debian.org/amule)
[![Debian CI](https://badges.debian.net/badges/debian/testing/amule/version.svg)](https://buildd.debian.org/amule)

[Forum]: https://github.com/amule-org/amule/discussions "aMule Forum"
[Wiki]:  https://github.com/amule-org/amule/wiki "aMule Wiki"
[FAQ]:   https://github.com/amule-org/amule/wiki/FAQ-aMule "FAQ on aMule"


## Overview

aMule is a multi-platform client for the eD2k / Kad file-sharing network,
originally a fork of the Windows client eMule (via xMule and lMule).
aMule started in August 2003.

Supported platforms today: Linux, FreeBSD, OpenBSD, macOS, and Windows
(MSYS2 / mingw-w64), on both x86_64 and ARM64.

aMule aims to stay close to eMule in look-and-feel so users moving between
the two have minimal friction. New eMule protocol-level features are
generally adopted into aMule shortly after.


## Features

* `amule` — all-in-one GUI client.
* `amuled` — headless daemon, no GUI.
* `amulegui` — remote GUI; connects to a local or remote `amuled` over the
  EC (External Connection) protocol.
* `amuleweb` — HTTP interface to a running `amuled`.
* `amulecmd` — interactive CLI for a running `amuled`.


## Installation

aMule ships pre-built binaries for every major desktop. Building
from source is also supported.

### Pre-built binaries (recommended)

Download the latest release for your platform from the
[Releases page]. Quick start:

* **Linux** — AppImage (any distro) or Flatpak: download, `chmod +x`, run.
* **macOS** — Universal2 `.dmg`: download, drag to `/Applications`.
* **Windows** — Portable `.zip` (x64 / ARM64): download, unzip, run.

See [docs/INSTALL_BINARIES.md](docs/INSTALL_BINARIES.md) for
per-platform notes — including the macOS unsigned-binary
workaround, the Windows SmartScreen prompt, and the Linux FUSE
dependency for AppImage.

[Releases page]: https://github.com/amule-org/amule/releases/latest

### Building from source

aMule uses CMake. Quick start:

```sh
cmake -B build -DBUILD_MONOLITHIC=YES -DBUILD_REMOTEGUI=YES
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

See [docs/INSTALL.md](docs/INSTALL.md) for the full list of dependencies,
build options (`BUILD_DAEMON`, `BUILD_AMULECMD`, `ENABLE_NLS`, `ENABLE_UPNP`,
`ENABLE_IP2COUNTRY`, etc.), and platform-specific notes. The CI workflow
[`.github/workflows/ccpp.yml`](.github/workflows/ccpp.yml) is the
authoritative reference for the exact deps and flags used to build aMule
on Linux, macOS, and Windows.


## Setting Up

aMule comes with reasonable default settings and should be usable as-is.
Two configuration steps are still worth doing on day one:

### 1. Open the ports — get a HighID

To receive a [HighID] you need to open aMule's ports on your firewall
and/or forward them on your router. The wiki has articles on
[getting a HighID][2] and [setting up firewall rules][3].

[HighID]: https://github.com/amule-org/amule/wiki/FAQ_eD2k‐Kademlia#what-is-lowid-and-highid "What is LowID and HighID?"
[2]: https://github.com/amule-org/amule/wiki/Get-HighID "How to get HighID"
[3]: https://github.com/amule-org/amule/wiki/Firewall "How to set up firewall rules for aMule"

### 2. Set bandwidth limits

aMule ships with both upload and download caps disabled by default
(`MaxUpload=0`, `MaxDownload=0` — both interpreted as literal
unlimited). On a connection that aMule can saturate, that means
aMule will eat all the bandwidth available to it, starving every
other application sharing the link. **Setting realistic limits is
strongly recommended.**

Under `Preferences → Connection`, set the limits to roughly **80 %
of your actual line speed** to avoid saturating the upstream and
starving your own traffic. Values are in **kilobytes per second**
(kB/s); ISP advertised speeds are usually in **megabits per
second** (Mbps). To convert, multiply Mbps by **125**.

> Example: a 100 Mbps / 20 Mbps fibre line → roughly 12 500 kB/s
> downstream and 2 500 kB/s upstream. Set the limits to about
> 10 000 down / 2 000 up to stay below the line cap.


## Reporting Bugs

If you find a bug or miss a feature, please open an issue on
[GitHub][5] (preferred) or report it on the [forum]. A good bug report
includes the exact aMule version (`amuled --version`), the platform you're
on, and steps to reproduce.

[5]: https://github.com/amule-org/amule/issues "aMule Issues"


## Contributing

*Contributions are always welcome!*

You can contribute to aMule in several ways:

* **Code** — fix a bug, implement a feature, improve performance. The preferred
  path is a [pull request][6] on GitHub; patches on the [forum] also work.
* **Translation** — [translate aMule][7], [translate the wiki][8], or
  [translate aMule's documentation][9] into your language.
* **Wiki** — aMule's wiki contains historical content that no longer matches
  current behaviour. Updating outdated pages is genuinely helpful.

[6]: https://github.com/amule-org/amule/pulls "aMule Pull Requests"
[7]: https://github.com/amule-org/amule/wiki/Translations "Translating aMule"
[8]: https://github.com/amule-org/amule/wiki/Translating-Wiki "Translating the wiki"
[9]: https://github.com/amule-org/amule/wiki/Translating-Docs "Translating the documentation"
