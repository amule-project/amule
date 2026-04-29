# IP2Country setup

aMule's *"Show country flags for clients"* feature uses a GeoIP
database to look up a country code from a peer's IP address and render
the matching flag in the client list, search results, transfers pane,
and shared-files-peers list.

The backing library is `libmaxminddb` and the database file is
`GeoLite2-Country.mmdb`.  This page documents the build-time and
runtime setup.


## Build prerequisites

`libmaxminddb` must be installed and discoverable by CMake's
`find_path(maxminddb.h)` and `find_library(maxminddb)` for the
IP2Country feature to be compiled in.

| Platform               | Install                                                  |
|------------------------|----------------------------------------------------------|
| Debian / Ubuntu        | `apt install libmaxminddb-dev`                           |
| Fedora / RHEL          | `dnf install libmaxminddb-devel`                         |
| Arch Linux             | `pacman -S libmaxminddb`                                 |
| macOS (Homebrew)       | `brew install libmaxminddb`                              |
| MSYS2 (Windows clang)  | `pacman -S mingw-w64-clang-aarch64-libmaxminddb` (ARM64) |
| MSYS2 (Windows MinGW)  | `pacman -S mingw-w64-x86_64-libmaxminddb` (x64)          |

If the library is not present at configure time, `ENABLE_IP2COUNTRY`
is set to `OFF` and the feature is omitted from the build with a
status message.  No runtime dependency is added.


## Where to put the .mmdb database

aMule does **not** ship the GeoLite2-Country database — it is updated
monthly by MaxMind under their own license and cannot be redistributed
inside the binary.

Drop the `GeoLite2-Country.mmdb` file into your aMule configuration
directory:

| Platform              | Path                                                                |
|-----------------------|---------------------------------------------------------------------|
| Linux                 | `~/.aMule/GeoLite2-Country.mmdb`                                    |
| macOS                 | `~/Library/Application Support/aMule/GeoLite2-Country.mmdb`         |
| Windows               | `%APPDATA%\aMule\GeoLite2-Country.mmdb`                             |
| Windows (portable)    | `<install-dir>\.aMule\GeoLite2-Country.mmdb` if a `.aMule` folder is next to `amule.exe`, otherwise the same `%APPDATA%` path |

aMule loads the file when the feature is enabled.  Replacing the file
requires an aMule restart.


## How to obtain the database

The free GeoLite2-Country database from MaxMind requires a (free)
account to download.  Three options:

### Option 1 — MaxMind direct (free account required)

1. Sign up at <https://www.maxmind.com/en/geolite2/signup>.
2. From the MaxMind portal, download *"GeoLite Country"* → *"GeoIP2
   Binary (.mmdb)"* → *"Download GZIP"* (not the CSV variant).
3. Decompress: `gunzip GeoLite2-Country*.mmdb.gz`.
4. Move the resulting `.mmdb` file into your aMule config directory
   (see table above).

### Option 2 — Third-party mirrors

Some community mirrors redistribute the same database.  Use at your
own risk and verify checksums when possible.  Searching for
"GeoLite2-Country.mmdb" turns up several maintained mirrors.

### Option 3 — Generate from CSV (advanced)

The CSV variant (`GeoLite2-Country-CSV`) is also free from MaxMind
and can be converted to the binary `.mmdb` format with the
[`mmdbctl`](https://github.com/ipinfo/mmdbctl) or
[`geolite2legacy`](https://github.com/sherpya/geolite2legacy) tools.


## Enabling the feature in aMule

1. Make sure `GeoLite2-Country.mmdb` is in the config directory above.
2. Open `Preferences` → `GUI Tweaks`.
3. Tick **"Show country flags for clients"**.
4. Apply / restart aMule.

Country flags should now appear in the search results, transfer
panes, and the shared-files-peers list when peers report their IP.


## Auto-update URL

GeoLite2-Country auto-update from inside aMule is awkward in
practice:

* MaxMind's official download URL requires a per-account license key
  embedded in the URL.
* The official archive is `.tar.gz` with a date-stamped subdirectory,
  which aMule's `UnpackArchive` does not handle (it understands
  `.gz` and `.zip` only).

The default value of `/eMule/GeoLiteCountryUpdateUrl` in
`amule.conf` is therefore **empty**, and `Update()` logs a clear
message asking for a manual download when the file is missing.

There is no GUI field for this URL.  Users who want to wire up an
auto-update endpoint can edit `amule.conf` directly while aMule is
not running.  Set the `GeoLiteCountryUpdateUrl` key under the
`[eMule]` section, e.g.:

```ini
[eMule]
GeoLiteCountryUpdateUrl=https://example.org/path/GeoLite2-Country.mmdb.gz
```

Path to `amule.conf`:

| Platform | Path                                              |
|----------|---------------------------------------------------|
| Linux    | `~/.aMule/amule.conf`                             |
| macOS    | `~/Library/Application Support/aMule/amule.conf`  |
| Windows  | `%APPDATA%\aMule\amule.conf`                      |

Notes on URLs:

* MaxMind direct (requires license key):
  `https://download.maxmind.com/app/geoip_download?edition_id=GeoLite2-Country&license_key=YOUR_LICENSE_KEY&suffix=tar.gz`
  — this does **not** currently work end-to-end because of the
  `.tar.gz` archive format.

* A third-party mirror that serves a plain `.mmdb.gz` works with the
  current download / decompress flow.


## Diagnostics

If the feature is enabled but flags don't appear:

* Confirm the file exists at the expected path and is non-empty.
* Confirm `Show country flags for clients` is ticked under
  `Preferences` → `GUI Tweaks`.
* Confirm aMule was built with IP2Country support: at startup the
  log line `aMule enabled options: …` reports `ENABLE_IP2COUNTRY` /
  `libmaxminddb` and the configured library path.
* Restart aMule after changing the database file (it is opened with
  `MMDB_MODE_MMAP` and not re-checked on the fly).

A log line of the form
```
Failed to open MaxMindDB database '…': <error>
```
indicates the file is missing, unreadable, or not a valid `.mmdb`.
