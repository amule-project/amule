# Flathub submission staging

This directory holds the **Flathub-strict** version of the aMule Flatpak
manifest — the file that will be submitted to
[github.com/flathub/flathub](https://github.com/flathub/flathub) for
inclusion in the Flathub catalogue, and (once approved) mirrored to
`github.com/flathub/org.amule.aMule` as the canonical Flathub-side
home.

The file in this directory is **not** consumed by the day-to-day
packaging build. That path uses
[`packaging/linux/flatpak/org.amule.aMule.yaml.in`](../linux/flatpak/org.amule.aMule.yaml.in)
— a template with `${...}` variables resolved at build time by
`packaging/linux/build.sh` via `envsubst`. The internal template uses
`branch: master` for aMule so dev / CI Flatpak builds always pick up
the current tip without manual updates.

## Why two manifests

Flathub's submission policy forbids mutable refs (branch names) and
requires every source to be pinned by an immutable identifier:

| Source type | Flathub-acceptable pin |
|---|---|
| `archive` | `sha256` |
| `git` (preferred) | `tag` + `commit` |
| `git` (minimal) | `commit` alone |
| `git` (forbidden) | `branch` |

Our internal template uses `branch: ${AMULE_REVISION}` so day-to-day
builds can target master or a feature branch. The Flathub-strict manifest
in this directory pins to a concrete tag + commit SHA so Flathub CI
produces reproducible builds.

Keeping both files in sync is a manual step. The differences are small
and localised:

- All `${...}` template variables resolved to concrete values.
- The `amule` module: `branch: ${AMULE_REVISION}` → `tag: + commit:`.
- The `cryptopp` module: existing `tag:` augmented with a `commit:`.

Every other module is identical between the two files (they were already
archive + sha256 pinned).

## Submission flow

1. Fork [`flathub/flathub`](https://github.com/flathub/flathub) on the
   maintainer's GitHub account.
2. Check out the `new-pr` branch on the fork.
3. Add a directory `org.amule.aMule/` containing:
   - `org.amule.aMule.yaml` — this manifest (copied verbatim).
   - The screenshots and icon are pulled at build time from the source
     `cmake --install` rule; no extra files needed in the Flathub
     directory.
4. Open a PR against `flathub/flathub:new-pr` titled
   `aMule (org.amule.aMule)`.
5. A Flathub volunteer reviewer will look at the manifest, may ask for
   changes (commonly: justification for `--filesystem=host`, app-id
   domain ownership confirmation), and merge when satisfied.
6. Once merged, Flathub creates a dedicated repo at
   `github.com/flathub/org.amule.aMule` and grants the submitter admin
   access. From that point the dedicated repo is the canonical home for
   the Flathub manifest.

## Refreshing after a new aMule release

When a new aMule release tag is pushed (e.g. `3.0.1`):

```sh
# resolve the new tag's commit SHA
gh api repos/amule-org/amule/git/refs/tags/3.0.1 --jq '.object.sha'
```

Update the `amule` source block in this file:

```yaml
- type: git
  url: https://github.com/amule-org/amule.git
  tag: '3.0.1'
  commit: <40-char SHA from the gh api call above>
```

Then PR the same change to `flathub/org.amule.aMule` (once that
repo exists). `x-checker-data` in the manifest lets Flathub's
`flatpak-external-data-checker` bot auto-open these PRs for us on
the Flathub side; the in-tree copy still needs a manual bump.

## Refreshing the runtime

GNOME Platform runtime bumps (currently pinned to `49`) need to be
coordinated with the internal template — both files reference the
same runtime version. Bump both at once and rebuild end-to-end.
