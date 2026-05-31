# Windows portable .zip + installer

Builds a portable aMule directory tree (`bin/{amule,amuled,amulegui,
amulecmd}.exe` + bundled MSYS2 DLLs + `ca-bundle.crt`) and zips it.
Users extract the zip and run `amule.exe`; no installer required.

An optional NSIS-driven installer .exe (see [Installer](#installer-exe)
below) wraps the same portable tree with shortcuts, an Add/Remove
Programs entry, and an opt-in "autostart on login" toggle for users
who prefer that flow.

## One-shot build

Runs INSIDE MSYS2 on a Windows machine. Two environments:

- **CLANGARM64** — Windows 11 ARM64 (default).
- **MINGW64** — Windows 10/11 x86_64.

```sh
# from repo root, on Windows in the right MSYS2 shell
packaging/windows/build.sh

# Result: ./dist/aMule-<version>-Windows-<arch>.zip
```

`packaging/windows/build.sh` reads `packaging/windows/versions.env` for
the target MSYSTEM and signing secrets, then drives cmake + ninja +
zip. Each invocation is idempotent; reuses `build-windows-<arch>/` if
present.

## Driving over SSH

If your Windows host's default OpenSSH shell is `bash -lc` (e.g. set
to MSYS2 with `MSYSTEM` exported), you can build remotely:

```sh
ssh <windows-host> '
cd <repo>
git fetch origin && git reset --hard origin/<branch>
packaging/windows/build.sh
'
# the produced .zip lives at <repo>/dist/ on the host
```

## Building for x86_64 instead of arm64

```sh
# inside MSYS2 MINGW64 shell, OR with the override:
WINDOWS_MSYSTEM=MINGW64 packaging/windows/build.sh
```

The script verifies that the shell's `MSYSTEM` matches `versions.env`
to prevent accidentally building the wrong arch (the wrong toolchain
in PATH would silently produce a working but mis-arch'd binary).

## What the recipe does

1. cmake configure with `-G Ninja -DCMAKE_BUILD_TYPE=Release` and the
   six `BUILD_*` flags aMule's portable shipper needs.
2. `cmake --install --prefix amule-portable-<arch>` produces a working
   tree with `bin/{amule,amuled,amulegui,amulecmd,ed2k}.exe` plus
   bundled MSYS2 DLLs (resolved via `file(GET_RUNTIME_DEPENDENCIES)`)
   and `bin/ca-bundle.crt` for libcurl HTTPS.
3. `taskkill` first — running aMule processes lock the .exe files so
   the install would fail with "Permission denied". Common when
   iterating on a single machine.
4. Zip the install tree at the parent dir level so the archive root
   is `amule-portable-<arch>/...`. Users extract once and get a single
   directory they can move anywhere.

## Code signing

Default ships **unsigned** — Windows SmartScreen warns the user but
allows opt-in via "More info → Run anyway". Set these env vars to
turn on signing without editing the recipe:

```sh
export WIN_CERT_PFX_BASE64=$(base64 -w0 path/to/cert.pfx)
export WIN_CERT_PASSWORD=cert-password

packaging/windows/build.sh sign     # signs every .exe + .dll, re-zips
```

When *either* var is unset, sign exits 0 silently. CI flips the
switch by setting both as repo secrets.

## Installer (.exe)

Wraps the portable tree from the previous step into a single
`aMule-<version>-Windows-Setup-<arch>.exe` driven by NSIS 3.x (MUI2). Produced
on demand:

```sh
packaging/windows/build.sh              # build the portable tree first
packaging/windows/build.sh installer    # then wrap it

# Result: ./dist/aMule-<version>-Windows-Setup-<arch>.exe
```

Requires `makensis` on `PATH`. NSIS isn't packaged for every MSYS2
environment (in particular there's no `clangarm64` build at the time
of writing), so install the Windows-native NSIS 3.x from
<https://nsis.sourceforge.io/> (or `choco install nsis`) and add its
install dir — typically `C:\Program Files (x86)\NSIS` — to `PATH`
before running the MSYS2 shell. The installer step re-uses the staged
`amule-portable-<arch>/` directory verbatim as its payload; it does
not re-resolve dependencies.

What the installer does:

- Stages the portable tree under `%ProgramFiles%\aMule` (default, user
  overridable on the Directory page).
- Creates Start Menu shortcuts for aMule (GUI), aMuleD (daemon),
  aMuleGUI (remote), and the uninstaller.
- Optional desktop shortcut (checked by default).
- Optional "Start aMule when I log in" toggle (unchecked by default)
  that writes `HKCU\…\Run\aMule` — per-user, so each user on a shared
  machine decides independently.
- Registers an Add/Remove Programs entry with publisher, version,
  install location, estimated size, and a working uninstaller.
- Uninstaller removes everything the installer placed, including the
  HKCU Run value (only when it still points inside the install dir,
  so a hand-set value isn't clobbered).

Per-machine install (requires admin). Coexists with the portable .zip
— different artifact name, different default install path; the two
are independent shipping channels.

Signing is the same one-switch flip as the .zip: set
`WIN_CERT_PFX_BASE64` + `WIN_CERT_PASSWORD` and either let the
`installer` subcommand sign the produced .exe inline (it does so
automatically when both vars are set), or run
`packaging/windows/build.sh sign` once — that subcommand signs both
the portable .zip and the installer .exe when both exist in `dist/`.

## Updating tool / dep pins

`packaging/windows/versions.env` carries:
- `WINDOWS_MSYSTEM` — default toolchain selection (CLANGARM64 or MINGW64)
- Signing variable name documentation

wxWidgets, cryptopp, libupnp etc. come from MSYS2's currently-installed
versions; we rely on MSYS2 as source of truth on Windows dev machines.
CI runners pin via `pacman -S --needed mingw-w64-<arch>-<pkg>` after
explicit version selection if reproducibility becomes an issue.
