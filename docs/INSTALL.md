# Installing aMule

## Requirements

aMule is built with [CMake](https://cmake.org). You'll need at least the
following packages:

| Package    | Minimum version | Notes |
| ---------- | --------------- | ----- |
| CMake      | 3.10            |       |
| zlib       | 1.2.3           |       |
| wxWidgets  | 3.2.0           | 3.2 branch or newer |
| Crypto++   | 5.6             |       |
| Boost      | 1.47            | headers only; only `asio` is used |

For `amuleweb` you'll also need a POSIX-compliant regex library — part of
the standard C library on most GNU systems.

### Optional dependencies

| Package                | Minimum | What it enables |
| ---------------------- | ------- | --------------- |
| `libgd`                | 2.0.0   | statistics images in `cas` |
| `libupnp`              | 1.6.6   | UPnP port forwarding |
| `libmaxminddb`         | 1.0     | country flags + IP→country mapping ([docs/IP2Country.md](IP2Country.md)) |
| `gettext`              | 0.11.5  | native-language support (NLS) |
| `libayatana-appindicator3` | —   | **Linux only.** StatusNotifierItem tray-icon backend. Without it the tray falls back to the legacy `GtkStatusIcon` API, which GNOME Shell dropped in 3.26 and wlroots compositors don't implement (silently invisible on Fedora / vanilla GNOME / Sway). Apt: `libayatana-appindicator3-dev`; RPM: `libayatana-appindicator-gtk3-devel`. |

### Linux-only note: glib-2.0 dev headers

The GUI build calls `g_set_prgname()` to bind the Wayland `wl_app_id` to
the `.desktop` filename, so glib-2.0 dev headers must be visible to
pkg-config. wxGTK transitively depends on glib but distro packaging
doesn't always pull the dev headers as a hard dep — install
`libglib2.0-dev` (Debian/Ubuntu) or `glib2-devel` (Fedora/RPM) explicitly
if pkg-config can't find `glib-2.0`.

### Unicode

aMule is unicode-only; wxWidgets must be built with unicode support
(this is the default since wx 3.0).


## Compiling aMule

The typical build is:

```sh
cmake -B build -DBUILD_MONOLITHIC=YES -DBUILD_REMOTEGUI=YES
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

By default files are installed under `/usr/local`:

| What | Where |
| ---- | ----- |
| Binaries | `/usr/local/bin/` |
| Translation catalogs | `/usr/local/share/locale/<lang>/LC_MESSAGES/amule.mo` |
| Data files, docs | `/usr/local/share/amule/` |
| Desktop entries | `/usr/local/share/applications/` |
| Icon | `/usr/local/share/icons/hicolor/128x128/apps/` |

Pass `--prefix=<dir>` to `cmake -B build` (or to `cmake --install`) to
install somewhere other than `/usr/local`. Installing under `$HOME/.local`
is useful during development — no `sudo` required and easy to clean up:

```sh
cmake --install build --prefix=$HOME/.local
```

Platform-specific notes (Homebrew paths on macOS, MSYS2 shells on
Windows, etc.) live in
[`.github/workflows/ccpp.yml`](../.github/workflows/ccpp.yml), which is
the authoritative reference used by CI.


## Uninstalling

CMake records every installed file in `build/install_manifest.txt`. Use
it to remove them:

```sh
sudo xargs rm -f < build/install_manifest.txt
```

If you installed under `$HOME/.local` no `sudo` is needed:

```sh
xargs rm -f < build/install_manifest.txt
```

### Linux-only icon-cache step

`cmake --install` ships `org.amule.aMule.png` into
`<prefix>/share/icons/hicolor/128x128/apps/`. Distro packages (`.deb` /
`.rpm`) refresh the GTK icon-theme cache automatically via post-install
scriptlets; a raw `cmake --install` does not. If the launcher / dock /
tray shows a generic placeholder ("three dots", question mark, etc.)
instead of the aMule mule icon immediately after install, refresh the
cache manually:

```sh
gtk-update-icon-cache -f -t <prefix>/share/icons/hicolor/
```

GNOME Shell's inotify watcher does pick up the new icons on its own
within a few seconds, so this is best-effort — the icon usually resolves
without the manual command.


## Build options

Common `-D` options (`YES` / `NO`):

| Option              | Default | Effect |
| ------------------- | ------- | ------ |
| `BUILD_MONOLITHIC`  | YES     | aMule GUI |
| `BUILD_REMOTEGUI`   | NO      | `amulegui` — remote control GUI |
| `BUILD_DAEMON`      | NO      | `amuled` — headless daemon |
| `BUILD_AMULECMD`    | NO      | `amulecmd` — CLI client for the daemon |
| `BUILD_WEBSERVER`   | NO      | `amuleweb` — HTTP interface for the daemon |
| `BUILD_ED2K`        | NO      | `ed2k` — handle `ed2k://` links |
| `BUILD_CAS`         | NO      | `cas` — C statistics tool |
| `BUILD_WXCAS`       | NO      | `wxCas` — GUI statistics tool |
| `BUILD_ALC`         | NO      | aMuleLinkCreator GUI |
| `BUILD_ALCC`        | NO      | aMuleLinkCreator console |
| `BUILD_FILEVIEW`    | NO      | console file viewer (experimental) |
| `ENABLE_NLS`        | YES     | native-language support (gettext) |
| `ENABLE_UPNP`       | YES     | UPnP port forwarding |
| `ENABLE_IP2COUNTRY` | NO      | libmaxminddb country flags ([docs/IP2Country.md](IP2Country.md)) |

For the full list:

```sh
cmake -LAH -B build | less
```


## Refreshing translated manpages (maintainers / translators)

The translated `*.LANG.1` manpages under `docs/man/` are committed
artifacts, regenerated from `docs/man/po/manpages-LANG.po` against the
English masters via `po4a`. `po4a` is not a build dependency — the
refresh target only exists when `po4a` is found at configure time:

```sh
cmake --build build --target po4a-update
```

This rewrites `docs/man/po/manpages.pot`, syncs each
`manpages-LANG.po`, and regenerates the translated `*.LANG.1` files in
place. Commit the resulting changes.


## Links

* Detailed build and usage information: <https://github.com/amule-org/amule/wiki>
* Forum for questions, bug reports, etc: <https://github.com/amule-org/amule/discussions>
* Upstream issue tracker: <https://github.com/amule-org/amule/issues>
