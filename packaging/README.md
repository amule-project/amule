# aMule packaging recipes

This tree builds aMule into the four user-facing distribution formats:
**AppImage**, **Flatpak**, **macOS Universal2 .dmg**, and **Windows
portable .zip**. All four are produced from a single `git` checkout
plus per-platform tooling (Docker / Homebrew / MSYS2).

The recipes are driven by [`.github/workflows/packaging.yml`](../.github/workflows/packaging.yml)
on GitHub Actions, but every script also runs locally — useful for
iteration, debugging, and producing one-off artifacts before a release.


## Tree layout

```
packaging/
├── linux/
│   ├── appimage/         AppImage recipe (Docker-driven)
│   ├── flatpak/          Flatpak manifest (template + generated yaml)
│   ├── build.sh          dispatcher — picks appimage or flatpak
│   └── versions.env      pinned tarball URLs + SHA256s for both
├── macos/                macOS Universal2 .dmg recipe
└── windows/              Windows portable .zip recipe (MSYS2)
```

Each platform subdir has its own `README.md` with platform-specific
build instructions and design notes:

| Platform | Recipe doc                                                            | Output format         |
| -------- | --------------------------------------------------------------------- | --------------------- |
| Linux    | [`linux/appimage/README.md`](linux/appimage/README.md)                | `.AppImage` (single file, glibc ≥ 2.35) |
| Linux    | [`linux/flatpak/README.md`](linux/flatpak/README.md)                  | `.flatpak` (sandboxed, GNOME 49 runtime) |
| macOS    | [`macos/README.md`](macos/README.md)                                  | `.dmg` (Universal2, arm64 + x86_64) |
| Windows  | [`windows/README.md`](windows/README.md)                              | `.zip` (portable, MSYS2 runtime bundled) |


## Building one platform locally

Each recipe is a single shell script with one entrypoint:

```sh
# Linux AppImage (host arch — needs Docker)
packaging/linux/build.sh appimage

# Linux Flatpak (host arch — needs flatpak-builder + GNOME runtime)
packaging/linux/build.sh flatpak

# macOS Universal2 .dmg (needs Homebrew with deps installed)
packaging/macos/build.sh

# Windows portable .zip (needs MSYS2 in CLANGARM64 or MINGW64 shell)
packaging/windows/build.sh
```

Output always lands in `./dist/` at the repo root. Per-platform
prerequisites (which Homebrew packages, which MSYSTEM, which Docker
base image) live in the per-platform `README.md` and `versions.env`.


## Building all four via GitHub Actions

The `.github/workflows/packaging.yml` workflow builds every platform
in parallel on its native runner — no QEMU emulation. Matrix:

| Platform       | Runner                  | Notes |
| -------------- | ----------------------- | ----- |
| AppImage x86_64 / aarch64 | `ubuntu-22.04` / `ubuntu-22.04-arm` | glibc 2.35 baseline for compat |
| Flatpak x86_64 / aarch64  | `ubuntu-24.04` / `ubuntu-24.04-arm` | needs `appstream-compose` 1.0+ from 24.04 |
| macOS arm64 / x86_64      | both on `macos-15`                  | x86_64 builds via Rosetta-emulated Homebrew at `/usr/local`; the legacy `macos-13` Intel runner is being retired by GitHub |
| macOS Universal2 .dmg     | `macos-15`                          | depends on the two macOS jobs; lipo-merges + ad-hoc-codesigns the result |
| Windows x64 / arm64       | `windows-latest`                    | MSYS2 MINGW64 / CLANGARM64 |

Triggers:

* **Push to `master`** (or any `packaging**` branch) when packaging /
  source / cmake files change — keeps master always-shippable.
* **Manual `workflow_dispatch`** — with an `only=appimage,flatpak,
  macos,windows` input for iterating on a single track.

Pull-request triggers are deliberately omitted to halve CI cost; if
you want to inspect a PR's artifacts before merging, dispatch the
workflow manually against the PR branch.


## Versions and dependency pins

`packaging/linux/versions.env`, `packaging/macos/versions.env`, and
`packaging/windows/versions.env` hold the tarball URLs, SHA256s, and
deployment-target values consumed by the recipes. Bumping a dep
version is a one-line edit in the right `versions.env`.

The Flatpak manifest is a template (`linux/flatpak/org.amule.aMule.yaml.in`);
`linux/build.sh` renders it via `envsubst` against `linux/versions.env`
to produce `linux/flatpak/org.amule.aMule.yaml`. Don't hand-edit the
generated `.yaml` — it gets overwritten on every build.


## What this repo does NOT ship

* `.spec` / `.deb` / Arch `PKGBUILD` files for distro packaging —
  distro maintainers handle those externally. This recipe set is
  upstream's "binary distribution" channel, complementary to distro
  packaging rather than replacing it.
* Code signing / notarization secrets. See `packaging/macos/sign.sh`
  for the stub; wiring up real Apple Developer credentials is a
  follow-up once the project obtains them.
* GitHub Releases. The current `packaging.yml` is a "PR sanity gate /
  master always-shippable" workflow; a tag-triggered `release.yml`
  that uploads to GitHub Releases is a planned follow-up.
