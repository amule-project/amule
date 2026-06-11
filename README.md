# aMule

![aMule](https://raw.githubusercontent.com/amule-org/amule/master/org.amule.aMule.png)

aMule is an eMule-like client for the eDonkey and Kademlia networks.

[Forum] | [Documentation] | [FAQ]

[Forum]:         https://github.com/amule-org/amule/discussions "aMule Forum"
[Documentation]: https://amule-org.github.io/docs "aMule Documentation"
[FAQ]:           https://amule-org.github.io/docs/manual/faq "FAQ on aMule"

## Important Notice

The work in aMule will continue in the new [aMule-org repo](https://github.com/amule-org/amule). The reason we had to create a new organization is that Gonosztopi, who is the single owner of [aMule Project](https://github.com/amule-project), has been unreachable. As a result, in the **aMule-project** organization we became unable to update the infrastructure to the project's needs.

## Overview

aMule is a multi-platform client for the eD2k / Kad file-sharing network,
originally a fork of the Windows client eMule (via xMule and lMule).
aMule started in August 2003.

Supported platforms today: Linux, FreeBSD, OpenBSD, NetBSD, macOS, and
Windows (MSYS2 / mingw-w64), on both x86_64 and ARM64.

aMule aims to stay close to eMule in look-and-feel so users moving between
the two have minimal friction. New eMule protocol-level features are
generally adopted into aMule shortly after.

---

| Distributions |
| --- |
| [![Arch Linux](https://repology.org/badge/version-for-repo/arch/amule.svg)](https://archlinux.org/packages/extra/x86_64/amule/) |
| [![AUR](https://repology.org/badge/version-for-repo/aur/amule.svg)](https://aur.archlinux.org/packages/amule) |
| [![Debian stable](https://badges.debian.net/badges/debian/stable/amule/version.svg)](https://buildd.debian.org/amule) |
| [![Debian testing](https://badges.debian.net/badges/debian/testing/amule/version.svg)](https://buildd.debian.org/amule) |
| [![FreeBSD](https://repology.org/badge/version-for-repo/freebsd/amule.svg)](https://www.freshports.org/net-p2p/amule/) |
| [![Gentoo](https://repology.org/badge/version-for-repo/gentoo/amule.svg)](https://packages.gentoo.org/packages/net-p2p/amule) |
| [![Kali Linux](https://repology.org/badge/version-for-repo/kali_rolling/amule.svg)](https://pkg.kali.org/pkg/amule) |
| [![Manjaro](https://repology.org/badge/version-for-repo/manjaro_stable/amule.svg)](https://repology.org/project/amule/versions) |
| [![NixOS 25.05](https://repology.org/badge/version-for-repo/nix_stable_25_05/amule.svg)](https://search.nixos.org/packages?channel=25.05&query=amule) |
| [![OpenBSD](https://repology.org/badge/version-for-repo/openbsd/amule.svg)](https://openports.pl/path/net/amule) |
| [![openSUSE Tumbleweed (Packman)](https://repology.org/badge/version-for-repo/packman_opensuse_tumbleweed/amule.svg)](http://packman.links2linux.org/package/aMule) |
| [![RPMFusion Fedora 42](https://repology.org/badge/version-for-repo/rpmfusion_fedora_42/amule.svg)](https://repology.org/project/amule/versions) |
| [![Slackware](https://repology.org/badge/version-for-repo/slackbuilds/amule.svg)](https://slackbuilds.org/result/?search=amule) |
| [![Solus](https://repology.org/badge/version-for-repo/solus/amule.svg)](https://repology.org/project/amule/versions) |
| [![Ubuntu 24.04 LTS](https://repology.org/badge/version-for-repo/ubuntu_24_04/amule.svg)](https://packages.ubuntu.com/noble/amule) |
| [![Ubuntu 25.04](https://repology.org/badge/version-for-repo/ubuntu_25_04/amule.svg)](https://packages.ubuntu.com/plucky/amule) |

---

## Development Statistics

| aMule-project (frozen) | [aMule-org](https://github.com/amule-org/amule) |
| ---------------------- | --------- |
| [![Open Pull Requests](https://img.shields.io/github/issues-pr/amule-project/amule)](https://github.com/amule-project/amule/pulls) | [![Open Pull Requests](https://img.shields.io/github/issues-pr/amule-org/amule)](https://github.com/amule-org/amule/pulls) |
| [![Open Issues](https://img.shields.io/github/issues/amule-project/amule)](https://github.com/amule-project/amule/issues) | [![Open Issues](https://img.shields.io/github/issues/amule-org/amule)](https://github.com/amule-org/amule/issues) |
| [![Bug](https://img.shields.io/github/issues/amule-project/amule/bug)](https://github.com/amule-project/amule/issues?q=is%3Aopen+is%3Aissue+label%3Abug) | [![Bug](https://img.shields.io/github/issues/amule-org/amule/bug)](https://github.com/amule-org/amule/issues?q=is%3Aopen+is%3Aissue+label%3Abug) |
| [![Feature Request](https://img.shields.io/github/issues/amule-project/amule/feature%20request)](https://github.com/amule-project/amule/issues?labels=feature+request) | [![Feature Request](https://img.shields.io/github/issues/amule-org/amule/feature%20request)](https://github.com/amule-org/amule/issues?labels=feature+request) |
| [![Enhancement](https://img.shields.io/github/issues/amule-project/amule/enhancement)](https://github.com/amule-project/amule/issues?labels=enhancement) | [![Enhancement](https://img.shields.io/github/issues/amule-org/amule/enhancement)](https://github.com/amule-org/amule/issues?labels=enhancement) |

## Features

* `amule` — all-in-one GUI client.
* `amuled` — headless daemon, no GUI.
* `amulegui` — remote GUI; connects to a local or remote `amuled` over the
  EC (External Connection) protocol.
* `amuleweb` — HTTP interface to a running `amuled`.
* `amulecmd` — interactive CLI for a running `amuled`.

## Installation

aMule ships pre-built binaries for every major desktop. Building from
source is also supported.

### Pre-built binaries (recommended)

Download the latest release for your platform from the
[Releases page]. Quick start:

* **Linux** — AppImage (any distro) or Flatpak: download, `chmod +x`, run.
* **macOS** — Universal2 `.dmg`: download, drag to `/Applications`.
* **Windows** — choose either the **NSIS installer** `.exe` (Start-menu shortcuts, uninstaller, x64 / ARM64) or the **portable `.zip`** (no install, unzip and run).

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
Two configuration steps are still worth doing on day one.

### Open the ports — get a HighID

To receive a [HighID] you need to open aMule's ports on your firewall
and/or forward them on your router. See the [network connectivity
guide][network] for details.

[HighID]:  https://amule-org.github.io/docs/p2p-networks/ed2k/high-id "What is LowID and HighID?"
[network]: https://amule-org.github.io/docs/manual/configuration/network-connectivity "Network connectivity"

### Set bandwidth limits

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
on, and steps to reproduce. See the [bug report guide][bug-report] for
detailed instructions on attaching backtraces and reproducer steps.

[5]:          https://github.com/amule-org/amule/issues "aMule Issues"
[bug-report]: https://amule-org.github.io/docs/contributing/bug-report "Bug Report Instructions"

## Contributing

*Contributions are always welcome!*

See the [contributing guide][contributing] for how to get involved. In short:

* **Code** — fix a bug, implement a feature, improve performance. The preferred
  path is a [pull request][6] on GitHub; patches on the [forum] also work.
* **Translation** — translate aMule, its documentation, or its website into
  your language.
* **Documentation** — help improve the project documentation at
  [amule-org.github.io/docs][Documentation].

[6]:            https://github.com/amule-org/amule/pulls "aMule Pull Requests"
[contributing]: https://amule-org.github.io/docs/contributing "Contributing to aMule"

## Translations

The translations of the application interface and the man pages live in this
repository and can be edited either by opening a pull request — see the
[Translations guide](https://amule-org.github.io/docs/developer/translations) —
or through [Weblate](https://hosted.weblate.org/projects/amule/), a translation
tool that stays in sync with git — see the
[Weblate guide](https://amule-org.github.io/docs/developer/translations/weblate).
