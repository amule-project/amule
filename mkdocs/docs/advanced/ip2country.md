# IP-to-Country

aMule can display a country flag next to each peer's IP address in the transfers and client lists. This feature requires the **MaxMind GeoLite2** database and the `libmaxminddb` library.

## Requirements

- aMule built with `-DENABLE_IP2COUNTRY=YES`
- The `GeoLite2-Country.mmdb` database file (not bundled — requires a free MaxMind account)

## Installing the Database

1. Register for a free account at [maxmind.com](https://www.maxmind.com/).
2. Download the **GeoLite2 Country** database in `.mmdb` format.
3. Place the file in the appropriate path:

| Platform | Path |
|---|---|
| Linux / BSD | `~/.aMule/GeoLite2-Country.mmdb` |
| macOS | `~/Library/Application Support/aMule/GeoLite2-Country.mmdb` |
| Windows | `%APPDATA%\aMule\GeoLite2-Country.mmdb` |

## Auto-Update

You can configure aMule to update the database automatically. Set the download URL in `amule.conf`:

```ini
[IPFilter]
AutoUpdateURL=https://download.maxmind.com/app/geoip_download?edition_id=GeoLite2-Country&license_key=YOUR_KEY&suffix=tar.gz
```

## Enabling in Preferences

Once the database file is in place, enable country flags in **Preferences → Remote Clients**.

## Note on Accuracy

The GeoLite2 database provides approximate country-level geolocation. It is not perfectly accurate and is provided for informational purposes only.
