#!/bin/bash
# Windows portable .zip recipe for aMule.
#
# Runs INSIDE MSYS2 on the Windows host (CLANGARM64 or MINGW64 env).
# Can also be driven remotely over SSH from another machine, e.g.:
#   ssh <windows-host> 'cd <repo> && packaging/windows/build.sh'

set -euo pipefail

usage() {
    cat >&2 <<EOF
usage: $0 [build|sign]

  build   default action: cmake configure → build → install portable tree →
          zip into dist/. Idempotent; reuses existing build/ if present.
  sign    signtool the .exe files inside the produced zip (and re-zip).
          Skipped automatically if WIN_CERT_* env vars aren't set.
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

VERSION=$(cd "${REPO_ROOT}" && git describe --tags --always --dirty 2>/dev/null || echo "snapshot")
ZIP_NAME="aMule-${VERSION}-Windows-${ARCH}.zip"

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

sign_zip() {
    "${SCRIPT_DIR}/sign.sh" "${DIST_DIR}/${ZIP_NAME}"
}

case "${1:-build}" in
    build) build ;;
    sign)  sign_zip ;;
    *)     usage ;;
esac
