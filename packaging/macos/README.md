# macOS .dmg

Builds a Universal2 (`x86_64;arm64`) .app bundle and packages it into a
`.dmg` for distribution. Targets macOS ≥ 11.0 (Big Sur) — first OS that
runs natively on Apple Silicon.

## One-shot build

```sh
# from repo root, on macOS
packaging/macos/build.sh

# Result: ./dist/aMule-<version>-macOS.dmg
```

The host needs:
- macOS 11+ with Xcode Command Line Tools
- Homebrew with: `cmake`, `wxwidgets`, `cryptopp`, `libupnp`, `gd`, `gettext`, `boost`, `dylibbundler`, `libmaxminddb`
- `hdiutil` (built into every macOS)

`packaging/macos/build.sh` sources `packaging/macos/versions.env` for the
deployment target and arch list, then drives cmake + dylibbundler + hdiutil.

## What the recipe does

1. `cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"` produces a fat
   `aMule.app` (and `aMuleGUI.app`) with both archs in each Mach-O.
2. `dylibbundler` walks each .app's main binary, copies all non-system
   `.dylib` deps into `Contents/libs/`, and rewrites the Mach-O load
   commands to `@executable_path/../libs/<name>.dylib`. The result
   runs on a clean macOS box without Homebrew installed.
3. The headless CLI binaries (`amuled`, `amulecmd`, `amuleweb`, `ed2k`)
   are copied into `aMule.app/Contents/MacOS/` next to the GUI binary,
   with their dylib deps bundled the same way. Power users can run
   `aMule.app/Contents/MacOS/amuled --version` from a terminal.
4. `hdiutil create … -format UDZO` produces a compressed read-only
   disk image with the .app bundle(s) and a symlink to `/Applications`
   so users see drag-to-install.

## Code signing + notarization

By default the .dmg ships **unsigned** — Gatekeeper warns the user but
allows opt-in via right-click → Open. To switch on signing without
editing any recipe:

```sh
export APPLE_DEVELOPER_ID="Developer ID Application: Name (TEAMID)"
export APPLE_TEAM_ID=ABCDE12345
export APPLE_NOTARY_USER=apple-id@example.com
export APPLE_NOTARY_PASS=xxxx-xxxx-xxxx-xxxx
export APPLE_CERT_P12_BASE64=$(base64 -i path/to/cert.p12)
export APPLE_CERT_PASSWORD=cert-password

packaging/macos/build.sh sign     # signs + notarizes the existing .dmg
```

When *any* of those env vars is unset, the sign step exits 0 silently —
local builds without certs Just Work. CI flips the switch by setting
all six as repo secrets.

## Updating tool / dep pins

`packaging/macos/versions.env` carries the build-time settings:
- `MACOS_DEPLOYMENT_TARGET` — bump up only if a dep requires it
- `MACOS_ARCHITECTURES` — keep `x86_64;arm64` for Universal2

Other version pins (wxWidgets, cryptopp, libupnp, libgd) come from
Homebrew's currently-installed versions; we deliberately don't pin them
here because Homebrew is the source of truth on macOS dev machines.
CI runners pin via `brew pin <pkg>` if reproducibility becomes an issue.
