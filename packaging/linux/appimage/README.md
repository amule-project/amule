# Linux AppImage

Builds a portable aMule binary that runs on every mainstream desktop Linux
distro current in 2026 (glibc ≥ 2.35: Ubuntu 22.04+, Debian 12+, Fedora 38+,
openSUSE Leap 15.5 / Tumbleweed, Arch, Mint, Pop!_OS, Steam Deck, Pi OS).

## One-shot build

The wrapper at `packaging/linux/build.sh` reads pinned versions from
`packaging/linux/versions.env` and drives Docker for you. Default mode
is **native** (host arch — fast, no QEMU):

```sh
# from repo root, build for the host's architecture
packaging/linux/build.sh appimage

# Result: ./dist/aMule-<version>-<arch>.AppImage
```

The host only needs Docker. Everything else (Ubuntu base image, wxWidgets
3.2.x build from source, linuxdeploy + GTK plugin) is pulled into a
self-contained image on first run.

## Building for a non-native arch

```sh
# Once per host: register QEMU binfmt handlers
packaging/linux/build.sh setup-cross-arch

# Then build for the other arch — uses QEMU emulation under the hood
packaging/linux/build.sh appimage x86_64    # on aarch64 host
packaging/linux/build.sh appimage aarch64   # on x86_64 host

# Result: ./dist/aMule-<version>-x86_64.AppImage  +  ./dist/aMule-<version>-aarch64.AppImage
```

Cross-arch via emulation is ~5-10× slower than native. Acceptable for one-off
artifact production locally; CI runs native runners on each arch (no QEMU).

If you skip `setup-cross-arch` and try a non-native build, the script
detects it up front and prints a one-line fix instead of failing mid-build.

## Building everything in one go

`packaging/linux/build.sh all` produces all 4 artifacts (appimage+flatpak
× x86_64+aarch64) sequentially. Cross-arch slots use QEMU; on a 4-core
aarch64 VM the full set is roughly **2.5–3 hours**. CI runs each slot
on its native runner in parallel and finishes much faster.

## How it works

1. `packaging/linux/build.sh appimage` sources `versions.env` and runs
   `docker build` with every pin passed as a `--build-arg`.
2. Inside the container, the entrypoint (`appimage/build.sh`) configures
   + builds aMule with `-DCMAKE_INSTALL_PREFIX=/usr`, then
   `cmake --install` with `DESTDIR=AppDir/` stages everything under
   `AppDir/usr/` in the XDG layout the desktop file + icon hooks already
   produce.
3. `linuxdeploy --plugin gtk` bundles wxGTK + GTK3 + every transitive .so,
   generates the AppRun launcher, and emits `aMule-<version>-<arch>.AppImage`
   in `dist/`.

The GTK plugin handles the wxGTK ↔ GTK3 ↔ glib ↔ pixman ↔ cairo dependency
chain that's the hard part of bundling a wx app — that's why
`linuxdeploy-plugin-gtk` is preferred over hand-rolling the bundle.

`--device /dev/fuse` + `--cap-add SYS_ADMIN` are passed by the wrapper
because `appimagetool` mounts the squashfs to compute the runtime delta;
without FUSE in the container it falls back to a slower in-memory path
that sometimes fails on large images.

## aarch64 build

Same `packaging/linux/build.sh appimage` invocation, run on an aarch64
host. GitHub Actions has native `ubuntu-22.04-arm` runners that avoid
the QEMU emulation penalty in CI.

## Validation matrix

Before tagging a release, the AppImage should be smoke-tested on:

- Ubuntu 22.04 (the build base, sanity check)
- Fedora 42 (RPM-side; different glibc patches)
- openSUSE Tumbleweed (rolling release; latest GTK)

A 30-second run is enough — boot the GUI, confirm Kad connects, exit.

## Updating tool/library pins

All pinned versions live in `packaging/linux/versions.env`. Bump there;
no other file should need editing for a routine refresh.

`LINUXDEPLOY_VERSION` is the most likely to need attention — alpha tags
get pruned over time, so an old pin will eventually 404 the build. Bump
to the latest tag at `https://github.com/linuxdeploy/linuxdeploy/releases`.
