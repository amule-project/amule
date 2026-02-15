# GeoIP Module for aMule

## Overview

This module provides modern IP to country lookup functionality for aMule, replacing the deprecated Legacy GeoIP library with a new implementation supporting MaxMind DB format.

## Features

- **Modern Database Format**: Supports MaxMind DB (`.mmdb`) format
- **Automatic Updates**: Built-in scheduler for database updates
- **Multiple Sources**: Configurable download sources with fallback
- **Progress Tracking**: Download progress monitoring
- **Backward Compatibility**: Legacy API wrapper maintained

## Dependencies

- **libmaxminddb**: Required for MaxMind DB format support
  - Install on Ubuntu/Debian: `sudo apt-get install libmaxminddb-dev`
  - Install on macOS: `brew install libmaxminddb`

## Building

### CMake Configuration

```cmake
# Enable GeoIP support
set(ENABLE_IP2COUNTRY ON)

# Find libmaxminddb
find_package(maxminddb REQUIRED)
```

### Build

```bash
mkdir build && cd build
cmake ..
make -j4
```

## Database Sources

The module supports multiple download sources in order of priority:

1. **GitHub Mirror (8bitsaver/maxmind-geoip)**
   - URL: `https://raw.githubusercontent.com/8bitsaver/maxmind-geoip/release/GeoLite2-Country.mmdb`
   - Priority: 0 (Highest)

2. **jsDelivr CDN**
   - URL: `https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb`
   - Priority: 1

3. **WP Statistics (npm package)**
   - URL: `https://cdn.jsdelivr.net/npm/geolite2-country/GeoLite2-Country.mmdb.gz`
   - Priority: 2

## Usage

### Programmatic Usage

```cpp
#include "geoip/IP2CountryManager.h"

// Get singleton instance
IP2CountryManager& manager = IP2CountryManager::GetInstance();

// Initialize with config directory
manager.Initialize("/home/user/.aMule/");

// Enable functionality
manager.Enable();

// Get country data for IP
CountryData data = manager.GetCountryData("192.168.1.1");

// Check for updates
manager.CheckForUpdates();
manager.DownloadUpdate();
```

### Legacy API Usage

The existing CIP2Country class is maintained for backward compatibility:

```cpp
#include "IP2Country.h"

CIP2Country ip2country(configDir);
ip2country.Enable();

CountryData data = ip2country.GetCountryData("192.168.1.1");
ip2country.Update();
```

## Database File Location

Default database path: `~/.aMule/GeoLite2-Country.mmdb`

## Configuration

### Preferences

- **GeoIPEnabled**: Enable/disable IP to country functionality
- **AutoUpdateEnabled**: Enable automatic database updates
- **UpdateCheckInterval**: Days between update checks (default: 7)

### Environment Variables

- `AMULE_GEOIP_PATH`: Override default database path

## Troubleshooting

### Database Not Found

If you see:
```
No GeoIP database found at: /home/user/.aMule/GeoLite2-Country.mmdb
```

Solution: Download the database manually:
```bash
wget -O ~/.aMule/GeoLite2-Country.mmdb \
  https://raw.githubusercontent.com/8bitsaver/maxmind-geoip/release/GeoLite2-Country.mmdb
```

### Update Failures

Check logs for details:
- Network errors: Verify internet connectivity
- Permission errors: Ensure write access to config directory
- Validation failures: Checksum mismatch

## License

This module is part of aMule and licensed under GPLv2.

MaxMind GeoLite2 database is licensed under CC BY-SA 4.0.
See: https://dev.maxmind.com/geolite2/geolite2-free-geolocation-data

## References

- MaxMind GeoLite2: https://dev.maxmind.com/geoip/geolite2-free-geolocation-data
- libmaxminddb: https://github.com/maxmind/libmaxminddb
- Alternative sources: https://github.com/8bitsaver/maxmind-geoip