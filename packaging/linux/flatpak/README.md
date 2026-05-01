# Flatpak

Manifest for building aMule as a Flatpak. Targets `org.gnome.Platform`
which provides GTK3 / glib / pixman / fontconfig / libpng / zlib / freetype.

`flatpak-external-data-checker` runs daily against the manifest (set up
in the Flathub repo's CI) and opens PRs when upstream releases for
`cryptopp`, `libupnp`, `libgd`, `wxWidgets`, or aMule itself are detected.

## Files

- `org.amule.aMule.yaml.in` — template; structural changes go here.
- `org.amule.aMule.yaml` — generated from `.in` + `versions.env`
  via `envsubst`. Don't hand-edit; it gets regenerated.

## App ID

`org.amule.aMule` is the canonical id — matches the AppStream component
id, the .desktop filename, the Wayland app_id (set in CamuleApp::OnInit
via g_set_prgname), and the macOS bundle id. Override via `APP_ID=` in
`packaging/linux/versions.env` if you ever need to fork; the manifest
template filename has to match `${APP_ID}.yaml.in`.

## Local build + smoke test

```sh
# Install runtime + SDK + binfmt for both arches (one-time)
packaging/linux/build.sh setup-cross-arch

# Build native (host arch)
packaging/linux/build.sh flatpak

# Cross-arch builds (uses QEMU emulation, slow)
packaging/linux/build.sh flatpak x86_64
packaging/linux/build.sh flatpak aarch64

# Result: ./dist/aMule-<version>-<arch>.flatpak (single bundle)

# Optional: install + run via the local repo
flatpak install --user --reinstall ./dist/aMule-<version>-<arch>.flatpak
flatpak run org.amule.aMule
```

To regenerate the rendered manifest without building:

```sh
packaging/linux/build.sh render-flatpak-manifest
```

To produce all 4 artifacts (appimage+flatpak × x86_64+aarch64) in one
go: `packaging/linux/build.sh all`.

## Pending before first build

The `libupnp` SHA256 in `packaging/linux/versions.env` is a placeholder.
On the first local build:

```sh
sha256sum <path-to-libupnp-release-tarball>
```

…and paste it into `versions.env`. After that, `flatpak-external-data-checker`
keeps all hashes in sync.

## Flathub submission

Once the manifest builds locally:

1. Open a PR against `flathub/flathub` adding a `new-pr/<app-id>` directory
   referencing the rendered manifest.
2. Address reviewer feedback (license, runtime version, finish-args).

After acceptance Flathub creates a `flathub/<app-id>` repo and rebuilds
on every manifest commit, and on every aMule release tag.
