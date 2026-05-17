# Installation

aMule is built with **CMake**. This page covers the required and optional dependencies and the build steps for each supported platform.

## Required Dependencies

| Package | Minimum Version | Notes |
|---|---|---|
| CMake | 3.10 | Build system |
| zlib | 1.2.3 | Compression |
| wxWidgets | 3.2.0 | GUI toolkit |
| Crypto++ | 5.6 | Cryptographic primitives |
| Boost | 1.70 | Headers only (ASIO) |

## Optional Dependencies

| Package | Feature enabled |
|---|---|
| `libgd` | Statistics graph images |
| `libupnp` | UPnP port mapping |
| `libmaxminddb` | IP-to-country flag display |
| `gettext` | Translations (NLS) |
| `libayatana-appindicator3` | Linux system tray (SNI) |

## Building

```sh
cmake -B build \
  -DBUILD_MONOLITHIC=YES \
  -DBUILD_DAEMON=YES \
  -DBUILD_REMOTEGUI=YES \
  -DBUILD_WEBSERVER=YES \
  -DBUILD_AMULECMD=YES
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

## CMake Build Options

| Option | Default | Description |
|---|---|---|
| `BUILD_MONOLITHIC` | YES | Build the `amule` GUI client |
| `BUILD_DAEMON` | NO | Build `amuled` headless daemon |
| `BUILD_REMOTEGUI` | NO | Build `amulegui` remote GUI |
| `BUILD_WEBSERVER` | NO | Build `amuleweb` HTTP interface |
| `BUILD_AMULECMD` | NO | Build `amulecmd` CLI |
| `BUILD_ED2K` | NO | Build `ed2k` link handler |
| `ENABLE_NLS` | YES | Enable gettext translations |
| `ENABLE_UPNP` | YES | Enable UPnP support |
| `ENABLE_IP2COUNTRY` | NO | Enable country flag display |

## Uninstalling

```sh
sudo xargs rm < build/install_manifest.txt
```
