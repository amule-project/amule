# Building from Source

aMule uses **CMake** as its build system. The minimum required version is CMake 3.10.

## Dependencies

See the full dependency table in [Installation](../getting-started/installation.md).

## Clone the Repository

```sh
git clone https://github.com/amule-project/amule.git
cd amule
```

## Configure

```sh
cmake -B build \
  -DBUILD_MONOLITHIC=YES \
  -DBUILD_DAEMON=YES \
  -DBUILD_REMOTEGUI=YES \
  -DBUILD_WEBSERVER=YES \
  -DBUILD_AMULECMD=YES \
  -DENABLE_NLS=YES \
  -DENABLE_UPNP=YES
```

To enable IP-to-country support:

```sh
  -DENABLE_IP2COUNTRY=YES
```

## Build

```sh
cmake --build build -j"$(nproc)"
```

## Install

```sh
sudo cmake --install build
```

Default install prefix is `/usr/local`. Override with `-DCMAKE_INSTALL_PREFIX=/your/prefix`.

## Running Without Installing

All built binaries are placed in `build/` and can be run directly from there for testing.

## Unit Tests

```sh
cmake -B build -DBUILD_TESTING=YES
cmake --build build
ctest --test-dir build
```

## Platform Notes

### Linux

Install dependencies via your distribution's package manager. The CI workflow (`.github/workflows/ccpp.yml`) lists the exact package names for Ubuntu.

### macOS

Use Homebrew to install wxWidgets, Boost, Crypto++, and other dependencies.

### Windows

Build using MSYS2 with the MINGW64 or CLANGARM64 toolchain. See the packaging scripts in `packaging/windows/` for reference.
