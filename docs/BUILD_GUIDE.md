# aMule Build Guide

## Table of Contents
- [Supported Platforms](#supported-platforms)
- [Requirements](#requirements)
- [Platform-Specific Build Instructions](#platform-specific-build-instructions)
- [Cross-Platform Building](#cross-platform-building)
- [Troubleshooting](#troubleshooting)

## Supported Platforms

- Linux (GCC/Clang)
- Windows (MSVC/MinGW)
- macOS (Clang)

## Requirements

### Common Requirements
- CMake >= 3.10
- C++ compiler with C++20 support
- wxWidgets >= 2.8.0 (2.8.9 or later recommended)
- zlib >= 1.1.4 (1.2.3 recommended)
- Crypto++ >= 5.1 (5.5.2 recommended)

### Optional Dependencies
- libgd (or gdlib) >= 2.0.0 - for statistics images
- libupnp >= 1.6.6 - for UPnP support
- libpng >= 1.2.0 - for webserver statistics graphs
- libGeoIP >= 1.4.4 - for country flags and IP to country mappings
- gettext >= 0.11.5 - for Native Language Support (NLS)

## Platform-Specific Build Instructions

### Linux

#### Installing Dependencies
**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libwxgtk3.0-gtk3-dev libcrypto++-dev zlib1g-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ cmake wxGTK3-devel crypto++-devel zlib-devel
```

#### Building
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Windows

#### Prerequisites
- Visual Studio 2019/2022 or MinGW-w64
- CMake
- wxWidgets (prebuilt or build from source)
- Crypto++ library

#### Building with Visual Studio
```bash
# Generate project files
cmake -G "Visual Studio 17 2022" -A x64 ..

# Build
cmake --build . --config Release
```

#### Building with MinGW
```bash
# Generate Makefiles
cmake -G "MinGW Makefiles" ..

# Build
mingw32-make -j$(nproc)
```

### macOS

#### Prerequisites
- Xcode Command Line Tools
- Homebrew (recommended for dependencies)

#### Installing Dependencies
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake wxwidgets crypto++ zlib
```

#### Building
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
make -j$(sysctl -n hw.ncpu)
```

#### Creating Application Bundle (Optional)
For creating a macOS application bundle, you can use the experimental scripts located in `src/utils/scripts/MacOSX`:

```bash
# From the root directory
mkdir Build
cd Build/
WXVERSION=svn WXPORT=cocoa MULECLEAN=YES ../src/utils/scripts/MacOSX/full_build.sh
```

This will create a macOS application bundle for aMule and aMuleGUI compatible with macOS 10.15 (Catalina) and later.

## Cross-Platform Building

### Using CMake Toolchains

CMake provides excellent cross-platform build support. The build system automatically detects and configures for your platform.

### Dependency Management
All platform dependencies are automatically managed by CMake, supporting:
- vcpkg (Windows)
- Homebrew (macOS)
- System package managers (Linux)

## Build Options

### Common CMake Options
```bash
cmake ..   -DCMAKE_BUILD_TYPE=Release   -DENABLE_AMULECMD=ON   -DENABLE_AMULEGUI=ON   -DENABLE_DAEMON=ON   -DENABLE_WEBSERVER=ON   -DENABLE_CAS=ON   -DENABLE_WXCAS=ON   -DENABLE_ALCC=ON   -DENABLE_ALC=ON
```

### Platform-Specific Options
- Linux: Default settings work for most distributions
- Windows: May need to specify wxWidgets path with `-DwxWidgets_ROOT_DIR`
- macOS: May need to set deployment target with `-DCMAKE_OSX_DEPLOYMENT_TARGET`

## Troubleshooting

### Common Issues

#### wxWidgets not found
**Solution:** Ensure wxWidgets development files are installed and specify the path:
```bash
cmake .. -DwxWidgets_ROOT_DIR=/path/to/wxWidgets
```

#### Crypto++ not found
**Solution:** Install Crypto++ development package or build from source:
```bash
# Ubuntu/Debian
sudo apt-get install libcrypto++-dev

# macOS
brew install crypto++
```

#### Compilation errors on macOS
**Solution:** Ensure you have the latest Xcode Command Line Tools:
```bash
xcode-select --install
```

#### Linking errors on Windows
**Solution:** Ensure all dependencies are built with the same compiler version and architecture (x86 or x64).

## Additional Resources

- [aMule Wiki](http://wiki.amule.org/wiki/Compile) - Detailed compilation instructions
- [aMule Forum](http://forum.amule.org) - Community support
- [aMule GitHub Issues](https://github.com/amule-project/amule/issues) - Bug reports and feature requests

## License

aMule is released under the GNU General Public License version 2. See the COPYING file for details.
