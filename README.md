# aMule

![aMule](https://raw.githubusercontent.com/amule-project/amule/master/org.amule.aMule.png)

aMule is an eMule-like client for the eDonkey and Kademlia networks.

[Forum] | [Wiki] | [FAQ]

[![Debian CI](https://badges.debian.net/badges/debian/stable/amule/version.svg)](https://buildd.debian.org/amule)
[![Debian CI](https://badges.debian.net/badges/debian/testing/amule/version.svg)](https://buildd.debian.org/amule)

[Forum]: http://forum.amule.org/ "aMule Forum"
[Wiki]:  http://wiki.amule.org/ "aMule Wiki"
[FAQ]:   http://wiki.amule.org/wiki/FAQ_aMule "FAQ on aMule"


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


## Compiling

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
However, to receive a [HighID] you need to open aMule's ports on your
firewall and/or forward them on your router. The wiki has articles on
[getting a HighID][2] and [setting up firewall rules][3].

[HighID]: http://wiki.amule.org/wiki/FAQ_eD2k-Kademlia#What_is_LowID_and_HighID.3F "What is LowID and HighID?"
[2]: http://wiki.amule.org/wiki/Get_HighID "How to get HighID"
[3]: http://wiki.amule.org/wiki/Firewall "How to set up firewall rules for aMule"


## Reporting Bugs

If you find a bug or miss a feature, please open an issue on
[GitHub][5] (preferred) or report it on the [forum]. A good bug report
includes the exact aMule version (`amuled --version`), the platform you're
on, and steps to reproduce.

[5]: https://github.com/amule-project/amule/issues "aMule Issues"


## Contributing

*Contributions are always welcome!*

You can contribute to aMule in several ways:

* **Code** — fix a bug, implement a feature, improve performance. The preferred
  path is a [pull request][6] on GitHub; patches on the [forum] also work.
* **Translation** — [translate aMule][7], [translate the wiki][8], or
  [translate aMule's documentation][9] into your language.
* **Wiki** — aMule's wiki contains historical content that no longer matches
  current behaviour. Updating outdated pages is genuinely helpful.

[6]: https://github.com/amule-project/amule/pulls "aMule Pull Requests"
[7]: http://wiki.amule.org/wiki/Translations "Translating aMule"
[8]: http://wiki.amule.org/wiki/Translating_Wiki "Translating the wiki"
[9]: http://wiki.amule.org/wiki/Translating_Docs "Translating the documentation"
