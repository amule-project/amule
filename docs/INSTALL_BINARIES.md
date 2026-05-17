# Installing aMule from binaries

Pre-built binaries are the simplest way to run aMule on Linux,
macOS, and Windows. Native packages for every supported platform
ship on the [Releases page][rel].

For building from source, see [INSTALL.md](INSTALL.md).

[rel]: https://github.com/amule-org/amule/releases/latest


## Contents

- [Linux](#linux)
  - [AppImage (recommended)](#appimage-recommended)
  - [Flatpak](#flatpak)
  - [Distro packages (older release)](#distro-packages-older-release)
- [macOS](#macos)
- [Windows](#windows)
- [Verifying the download](#verifying-the-download)
- [Running the headless tools](#running-the-headless-tools)
  - [Windows portable .zip](#windows-portable-zip)
  - [macOS .dmg](#macos-dmg)
  - [Linux AppImage](#linux-appimage)
  - [Linux Flatpak](#linux-flatpak)


## Linux

### AppImage (recommended)

A single self-contained binary that runs on any modern distro
(glibc ≥ 2.31). Both `x86_64` and `aarch64` builds are published.

1. Download `aMule-<version>-<arch>.AppImage` from the
   [Releases page][rel].
2. Make it executable and run:

   ```sh
   chmod +x aMule-*-*.AppImage
   ./aMule-*-*.AppImage
   ```

On first launch aMule offers to install a `.desktop` entry into
your application menu (opt-in, reversible — declined runs and the
"Don't ask again" choice are remembered).

**Pitfall — FUSE missing:** if the AppImage exits immediately with
`AppImages require FUSE to run`, install it:

* Debian / Ubuntu: `sudo apt install libfuse2t64` (or `libfuse2`)
* Fedora: `sudo dnf install fuse`
* Arch: `sudo pacman -S fuse2`

### Flatpak

Sandboxed install via `flatpak`. Both `x86_64` and `aarch64` builds
are published.

1. Download `aMule-<version>-<arch>.flatpak` from the
   [Releases page][rel].
2. Install and run:

   ```sh
   flatpak install --user aMule-*.flatpak
   flatpak run org.amule.aMule
   ```

The Flatpak runs against the GNOME Platform 47 runtime, but the
manifest grants `--filesystem=home` so aMule has full access to
your home directory and stores state in `~/.aMule/` — the same
path as a native install. Existing config, shared files, and
partfiles from a previous non-Flatpak install are picked up
automatically.

### Distro packages (older release)

Major distros ship aMule via their official repos, but the version
is typically the older 2.3.3 (2021) release rather than the
upstream 3.0.0+ binaries above. Use distro packages only if you
specifically want the distro-maintained build.

* Debian / Ubuntu: `sudo apt install amule amule-daemon`
* Fedora: `sudo dnf install amule`


## macOS

A Universal2 `.dmg` covers both Apple Silicon and Intel Macs in
one download.

1. Download `aMule-<version>-macOS-universal2.dmg` from the
   [Releases page][rel].
2. Open the `.dmg`, drag `aMule.app` to `/Applications`.

**Pitfall — "aMule cannot be opened because the developer cannot
be verified":** the binary is unsigned (the project doesn't have
an Apple Developer Program subscription). The procedure to allow
it once depends on the macOS version.

**macOS 15 (Sequoia) and newer**

Apple removed the Control-click → Open bypass in macOS 15. The
supported path is now:

1. Double-click `aMule.app` — the warning dialog appears, click
   **Done** to dismiss.
2. Open **System Settings → Privacy & Security**.
3. Scroll to the security message about aMule being blocked, click
   **Open Anyway**.
4. Re-launch aMule and confirm in the dialog. macOS remembers the
   exception; subsequent launches go straight in.

**macOS 14 (Sonoma) and earlier**

* **Control-click `aMule.app` → Open → Open** in the dialog. macOS
  remembers the exception; subsequent launches go straight in.

**Terminal alternative (any macOS version)**

Strip the quarantine attribute, no UI clicks required:

```sh
xattr -d com.apple.quarantine /Applications/aMule.app
```


## Windows

A portable `.zip` that just needs unzipping. Both `x64` and
`ARM64` builds are published.

1. Download `aMule-<version>-Windows-<arch>.zip` from the
   [Releases page][rel].
2. Unzip anywhere (e.g., `C:\Program Files\aMule\` or
   `%USERPROFILE%\amule\`).
3. Run `aMule.exe` (GUI) or `amuled.exe` (headless daemon).

**Pitfall — Windows Defender SmartScreen "Windows protected your
PC":** the binary is unsigned. Click **More info** → **Run anyway**
to allow it. SmartScreen learns over time as more users run the
same release.

The `.zip` already bundles every dependency (wxWidgets, Boost
ASIO, Crypto++, libcurl with CA bundle); no separate runtime
install is needed.


## Verifying the download

GitHub serves all release artifacts over HTTPS, and the Releases
page lists the SHA-256 of each asset (click "Show" next to the
filename). To verify locally:

```sh
sha256sum aMule-*-*.AppImage          # Linux
shasum -a 256 aMule-*-*.dmg            # macOS
certutil -hashfile aMule-*.zip SHA256  # Windows
```

Compare against the value on the release page.


## Running the headless tools

aMule ships several binaries that share the same `~/.aMule/` state:
the GUI (`amule`), the headless daemon (`amuled`), the remote GUI
(`amulegui`), the web UI (`amuleweb`), and the CLI (`amulecmd`).
For day-to-day desktop use the GUI is enough; the rest are for
running aMule on a NAS / VPS / always-on box and connecting from
elsewhere.

The user-guide section
[Running aMule headless](README.md#running-amule-headless-amuled)
covers the first-run setup (EC password, daemon flags). Its example
commands assume `amuled` / `amulecmd` / `amulegui` / `amuleweb`
resolve on `$PATH`, which is true for a `make install` build but
not for any of the binary packages below — each one stages the
binaries differently.

### Windows portable .zip

All binaries sit alongside each other in the unzipped folder. Open
PowerShell or Command Prompt in that folder and run them directly:

```
.\amuled.exe --full-daemon
.\amulecmd.exe -h 127.0.0.1 -p 4712 -P <password>
.\amulegui.exe
.\amuleweb.exe --admin-pass=<password>
```

### macOS .dmg

`aMule.app/Contents/MacOS/` holds the GUI plus `amuled`,
`amulecmd`, `amuleweb`, and `ed2k`; invoke them by full path:

```sh
/Applications/aMule.app/Contents/MacOS/amuled --full-daemon
/Applications/aMule.app/Contents/MacOS/amulecmd -h 127.0.0.1 -p 4712 -P <password>
/Applications/aMule.app/Contents/MacOS/amuleweb --admin-pass=<password>
```

`amulegui` ships as a separate `aMuleGUI.app` bundle in the `.dmg`
— drag both bundles to `/Applications` and double-click
`aMuleGUI.app`, or run
`/Applications/aMuleGUI.app/Contents/MacOS/aMuleGUI` from a
terminal.

To get the short command names back, drop symlinks into a directory
already on `$PATH`:

```sh
sudo ln -s /Applications/aMule.app/Contents/MacOS/amuled       /usr/local/bin/amuled
sudo ln -s /Applications/aMule.app/Contents/MacOS/amulecmd     /usr/local/bin/amulecmd
sudo ln -s /Applications/aMule.app/Contents/MacOS/amuleweb     /usr/local/bin/amuleweb
sudo ln -s /Applications/aMuleGUI.app/Contents/MacOS/aMuleGUI  /usr/local/bin/amulegui
```

### Linux AppImage

The AppImage dispatches on its own filename — symlink it to the
tool name you want and invoke the symlink:

```sh
ln -s aMule-3.0.0-x86_64.AppImage amuled
./amuled --full-daemon

ln -s aMule-3.0.0-x86_64.AppImage amulecmd
./amulecmd -h 127.0.0.1 -p 4712 -P <password>

ln -s aMule-3.0.0-x86_64.AppImage amulegui
./amulegui
```

Arch suffixes on the symlink are tolerated (`amuled-x86_64`,
`amulecmd-aarch64`, etc.), as is a trailing `.AppImage`.

### Linux Flatpak

The default command for `org.amule.aMule` is the GUI; pick a
different binary with `--command=`:

```sh
flatpak run --command=amuled    org.amule.aMule --full-daemon
flatpak run --command=amulecmd  org.amule.aMule -h 127.0.0.1 -p 4712 -P <password>
flatpak run --command=amulegui  org.amule.aMule
flatpak run --command=amuleweb  org.amule.aMule --admin-pass=<password>
```

The Flatpak grants `--filesystem=home`, so `~/.aMule/` is shared
with native installs and other package formats on the same machine.
