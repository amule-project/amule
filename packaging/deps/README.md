# Vendored build dependencies

This directory contains source tarballs for build dependencies whose
canonical upstream URLs have proven unreliable enough that pinning the
manifest to fetch from them at build time produces flaky CI.

The current scope is **launchpad.net** — both files below are only
hosted there and the launchpad CDN times out / 503s often enough to
warrant local copies. Other deps (cryptopp / wxwidgets / boost / libgd /
pupnp / libmaxminddb / AyatanaIndicators) come from GitHub release
assets, which have not been a problem.

## Inventory

| File                          | Size  | sha256 (truncated)         | Upstream URL                                                                                              |
| ----------------------------- | ----- | -------------------------- | --------------------------------------------------------------------------------------------------------- |
| `intltool-0.51.0.tar.gz`      | 158 K | `67c74d94…b334e959cd`      | <https://launchpad.net/intltool/trunk/0.51.0/+download/intltool-0.51.0.tar.gz>                            |
| `libdbusmenu-16.04.0.tar.gz`  | 743 K | `b9cc4a2a…1bfa878a`        | <https://launchpad.net/libdbusmenu/16.04/16.04.0/+download/libdbusmenu-16.04.0.tar.gz>                    |

The full sha256 sums are pinned in `packaging/linux/flatpak/org.amule.aMule.yaml.in`,
right next to each `path:` source. flatpak-builder verifies them at
extract time, so a corrupted vendored copy fails the build instead of
silently producing a wrong artifact.


## Updating

When upstream releases a new version of either package and we want to
adopt it:

1. `curl` the new tarball from upstream and place it in this directory
   under the new filename.
2. Update the matching `path:` and `sha256:` in
   `packaging/linux/flatpak/org.amule.aMule.yaml.in`.
3. Update this README's table.
4. Delete the old tarball.
5. Test on the fork before submitting upstream.

Both packages are essentially abandoned upstream (intltool's last
release was 2015, libdbusmenu's was 2016), so updates are unlikely.


## Why not Git LFS / GitHub release assets?

Both files combined are < 1 MB, which doesn't justify the operational
cost of either alternative:

* **Git LFS** would add a runtime dependency on `git-lfs` for everyone
  cloning the repo and is constrained by GitHub's free-tier bandwidth
  quota.
* **GitHub release assets** would require a release-cutting workflow
  and a moving "vendored-deps-YYYY.MM.DD" tag whose URLs the manifest
  would need to track.

Plain in-tree files keep clones self-contained and the build path
trivially reproducible without any extra tooling. If the dep set ever
grows past ~10 MB, revisiting either approach becomes worthwhile.


## Provenance verification

If you ever need to confirm the bytes here match what upstream
published, the canonical sha256s are recorded in upstream tarball
manifests / package metadata across distros:

* Debian's package source for libdbusmenu pins the same `b9cc4a2a…`
  for `libdbusmenu_16.04.0.orig.tar.gz`.
* The intltool 0.51.0 sha256 `67c74d94…` is identical across
  Fedora's lookaside cache, Debian's archive, and the original
  launchpad release notes.
