# Changelog

## 3.0.0 — "alive again" (2026)

First major release in over five years.

### Highlights

- **Throughput rewrite**: approximately 100–380× speedup compared to 2.3.3 on macOS, Linux, and Windows. Disk I/O for both uploads and downloads is now offloaded to dedicated background threads (`CUploadDiskIOThread`, `CPartFileWriteThread`).
- **CMake replaces autotools**: the entire build system has been rewritten in CMake (minimum 3.10). wxWidgets minimum version raised to 3.2.
- **Native distribution packages**: AppImage (x86\_64 + aarch64), Flatpak (x86\_64 + aarch64), macOS Universal2 `.dmg`, and Windows portable `.zip` (x64 + ARM64).
- **HTTPS fixed**: `CHTTPDownloadThread` rewritten on `wxWebRequest`; server list and IP filter auto-updates now work correctly over HTTPS.
- **Kademlia parallel searches**: alpha-frontier widening for faster and more thorough Kad lookups.
- **MaxMindDB**: replaces the deprecated GeoIP library for IP-to-country lookups.
- 98 merged pull requests.

---

## 2.3.3 (2021-02-07)

- wxWidgets 3.1 compatibility fixes.
- C++17 build support.
- UPnP API fix.
- Download completion notifications.
- Various crash fixes and minor improvements.

---

## 2.3.2 (2016-09-16)

- Stability and compatibility improvements.
- Updated translations.

---

## 2.3.1 (2011-11-11)

- Numerous bug fixes across networking, GUI, and build system.

---

Older entries are available in the [`docs/CHANGELOG.md`](https://github.com/amule-project/amule/blob/master/docs/CHANGELOG.md) file in the source repository.
