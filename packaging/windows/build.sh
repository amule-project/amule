#!/bin/bash
# Windows portable .zip recipe for aMule.
#
# Runs INSIDE MSYS2 on the Windows host (CLANGARM64 or MINGW64 env).
# Can also be driven remotely over SSH from another machine, e.g.:
#   ssh <windows-host> 'cd <repo> && packaging/windows/build.sh'

set -euo pipefail

usage() {
    cat >&2 <<EOF
usage: $0 [build|installer|sign]

  build      default action: cmake configure → build → install portable tree →
             zip into dist/. Idempotent; reuses existing build/ if present.
  installer  build the NSIS installer .exe on top of an already-staged
             portable tree (run \`$0\` first). Idempotent.
  sign       signtool the .exe files inside the produced zip (and the
             installer .exe if present). Skipped silently when WIN_CERT_*
             env vars aren't set.
EOF
    exit 64
}

# When invoked over SSH, run in foreground. Do NOT wrap in
# `nohup bash -c '…'` on Windows — nohup's stdio redirection breaks
# console-attachment in cmake's try_compile inner subprocesses, and
# every libc header check fails with "Generator: build tool execution
# failed". Foreground SSH is enough to keep the build alive across
# short network blips; for genuine detachment use `screen` or `tmux`
# on the host, not nohup.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
VERSIONS_ENV="${SCRIPT_DIR}/versions.env"

[[ -f "${VERSIONS_ENV}" ]] || { echo "fatal: ${VERSIONS_ENV} missing" >&2; exit 1; }

# shellcheck disable=SC1090
set -a; source "${VERSIONS_ENV}"; set +a

# Resolve filename arch suffix from MSYSTEM. Match the convention
# AppImage uses (x86_64 / aarch64) so the same naming pattern works
# across all platforms in dist/.
case "${WINDOWS_MSYSTEM}" in
    CLANGARM64) ARCH=arm64 ;;
    MINGW64)    ARCH=x64 ;;
    *) echo "fatal: unsupported WINDOWS_MSYSTEM=${WINDOWS_MSYSTEM}" >&2; exit 1 ;;
esac

BUILD_DIR="${REPO_ROOT}/build-windows-${ARCH}"
PORTABLE_DIR="${REPO_ROOT}/amule-portable-${ARCH}"
DIST_DIR="${REPO_ROOT}/dist"
mkdir -p "${DIST_DIR}"

VERSION=$(cd "${REPO_ROOT}" && git describe --tags --always 2>/dev/null || echo "snapshot")
ZIP_NAME="aMule-${VERSION}-Windows-${ARCH}.zip"
INSTALLER_NAME="aMule-${VERSION}-Windows-Setup-${ARCH}.exe"

require_tool() {
    command -v "$1" >/dev/null || {
        echo "fatal: '$1' not on PATH (need pacman -S $2)" >&2
        exit 1
    }
}

build() {
    require_tool cmake "mingw-w64-${ARCH}-cmake"
    require_tool mingw32-make "mingw-w64-${ARCH}-make"
    require_tool zip   "zip"

    # Verify we're in the right MSYS2 env. The MSYSTEM env var should
    # match versions.env; if a user runs from MINGW64 with versions.env
    # set to CLANGARM64 (or vice versa), the build will silently produce
    # the wrong arch.
    if [[ "${MSYSTEM:-}" != "${WINDOWS_MSYSTEM}" ]]; then
        echo "fatal: shell MSYSTEM=${MSYSTEM:-unset} but versions.env wants ${WINDOWS_MSYSTEM}" >&2
        echo "       launch the matching MSYS2 environment first, or override WINDOWS_MSYSTEM" >&2
        exit 1
    fi

    echo "==> Configuring cmake (Windows ${ARCH}, ${WINDOWS_MSYSTEM})"
    # Generator choice: "MinGW Makefiles" picks mingw32-make.exe from
    # the clangarm64 toolchain — matching the native Windows compiler.
    # Other choices break:
    # * "Ninja"          — ninja crashes with 0xc0000142 in cmake's
    #                      try_compile inner-project context. Same on
    #                      CCACHE-enabled and disabled paths.
    # * "Unix Makefiles" — picks /usr/bin/make.exe (MSYS2 POSIX
    #                      subsystem), which builds in a context where
    #                      every libc header check fails.
    # ccache also gets disabled here; on CLANGARM64 it triggered the
    # same family of try_compile crashes that pushed us off Ninja.
    cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}" -G "MinGW Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        -DBUILD_MONOLITHIC=YES \
        -DBUILD_REMOTEGUI=YES \
        -DBUILD_DAEMON=YES \
        -DBUILD_AMULECMD=YES \
        -DBUILD_ED2K=YES \
        -DBUILD_TESTING=NO \
        -DENABLE_IP2COUNTRY=YES

    echo "==> Building"
    cmake --build "${BUILD_DIR}" -j"$(nproc)"

    # Kill any running aMule before install — the .exe files in the
    # destination would otherwise be locked. Common when iterating.
    taskkill //F //IM amule.exe //IM amulegui.exe //IM amuled.exe 2>/dev/null || true

    echo "==> Installing portable tree to ${PORTABLE_DIR}"
    rm -rf "${PORTABLE_DIR}"
    cmake --install "${BUILD_DIR}" --prefix "${PORTABLE_DIR}"

    echo "==> Producing ${ZIP_NAME}"
    cd "$(dirname "${PORTABLE_DIR}")"
    rm -f "${DIST_DIR}/${ZIP_NAME}"
    # -r recursive, -q quiet, -X strip extra metadata. Path inside
    # zip is "amule-portable-<arch>/..." — users extract it and get
    # a single root dir.
    zip -rqX "${DIST_DIR}/${ZIP_NAME}" "$(basename "${PORTABLE_DIR}")"

    local size_mb=$(( $(stat -c%s "${DIST_DIR}/${ZIP_NAME}" 2>/dev/null || stat -f%z "${DIST_DIR}/${ZIP_NAME}") / 1024 / 1024 ))
    echo "==> Produced ${DIST_DIR}/${ZIP_NAME} (${size_mb} MB)"
}

installer() {
    # NSIS isn't packaged for every MSYS2 environment (no clangarm64
    # build at the time of writing), so the recommended path is the
    # Windows-native installer from https://nsis.sourceforge.io/.
    # MSYS2's default path-type=minimal drops the Windows PATH from
    # the bash session — auto-discover at the standard install dir so
    # callers (CI and dev machines alike) don't have to plumb $PATH.
    if ! command -v makensis >/dev/null; then
        for d in "/c/Program Files (x86)/NSIS" "/c/Program Files/NSIS"; do
            if [[ -x "$d/makensis.exe" ]]; then
                export PATH="$d:${PATH}"
                break
            fi
        done
    fi
    if ! command -v makensis >/dev/null; then
        echo "fatal: 'makensis' not on PATH and not found at standard install dirs." >&2
        echo "       install NSIS 3.x from https://nsis.sourceforge.io/" >&2
        echo "       (or 'choco install nsis'), then re-run." >&2
        exit 1
    fi

    if [[ ! -d "${PORTABLE_DIR}" ]]; then
        echo "fatal: portable tree ${PORTABLE_DIR} not found." >&2
        echo "       run '$0' first to build the portable .zip payload." >&2
        exit 1
    fi
    if [[ ! -x "${PORTABLE_DIR}/bin/amule.exe" ]]; then
        echo "fatal: ${PORTABLE_DIR}/bin/amule.exe missing — portable tree is incomplete." >&2
        exit 1
    fi

    # Copy the project license into the portable tree so the MUI2
    # License page can reference a path under INSTROOT (keeps the
    # installer self-contained and the .nsi relocatable).
    cp "${REPO_ROOT}/docs/COPYING" "${PORTABLE_DIR}/COPYING.txt"

    local out="${DIST_DIR}/${INSTALLER_NAME}"
    rm -f "${out}"

    echo "==> Producing ${INSTALLER_NAME}"
    makensis \
        "-DINSTROOT=${PORTABLE_DIR}" \
        "-DARCH=${ARCH}" \
        "-DVERSION=${VERSION}" \
        "-DOUTFILE=${out}" \
        "-DICONFILE=${REPO_ROOT}/amule.ico" \
        "${SCRIPT_DIR}/installer.nsi"

    local size_mb=$(( $(stat -c%s "${out}" 2>/dev/null || stat -f%z "${out}") / 1024 / 1024 ))
    echo "==> Produced ${out} (${size_mb} MB)"

    # Auto-sign when both secrets are populated; silent no-op otherwise.
    if [[ -n "${WIN_CERT_PFX_BASE64:-}" && -n "${WIN_CERT_PASSWORD:-}" ]]; then
        "${SCRIPT_DIR}/sign.sh" "${out}"
    fi
}

sign_artifacts() {
    "${SCRIPT_DIR}/sign.sh" "${DIST_DIR}/${ZIP_NAME}"
    # Sign the installer too when present, so a single `sign` covers
    # both shipping artifacts.
    if [[ -f "${DIST_DIR}/${INSTALLER_NAME}" ]]; then
        "${SCRIPT_DIR}/sign.sh" "${DIST_DIR}/${INSTALLER_NAME}"
    fi
}

case "${1:-build}" in
    build)     build ;;
    installer) installer ;;
    sign)      sign_artifacts ;;
    *)         usage ;;
esac
