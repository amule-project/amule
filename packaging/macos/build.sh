#!/bin/bash
# macOS Universal2 .dmg recipe for aMule.
#
# Usage:
#   packaging/macos/build.sh           # build + bundle + dmg
#   packaging/macos/build.sh sign      # sign + notarize a previously-built .dmg
#                                      # (no-op when APPLE_* env vars are unset)

set -euo pipefail

usage() {
    cat >&2 <<EOF
usage: $0 [build|sign]

  build   default action: cmake configure (Universal2) → build → bundle dylibs
          → create .dmg in dist/. Idempotent.
  sign    codesign the produced .app / .dmg and submit for notarization.
          Skipped automatically if APPLE_* env vars aren't set.
EOF
    exit 64
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
VERSIONS_ENV="${SCRIPT_DIR}/versions.env"

[[ -f "${VERSIONS_ENV}" ]] || { echo "fatal: ${VERSIONS_ENV} missing" >&2; exit 1; }

# shellcheck disable=SC1090
set -a; source "${VERSIONS_ENV}"; set +a

BUILD_DIR="${REPO_ROOT}/build-macos"
DIST_DIR="${REPO_ROOT}/dist"
mkdir -p "${DIST_DIR}"

VERSION=$(cd "${REPO_ROOT}" && git describe --tags --always --dirty 2>/dev/null || echo "snapshot")
APP_BUNDLE="${BUILD_DIR}/src/aMule.app"
GUI_BUNDLE="${BUILD_DIR}/src/aMuleGUI.app"

# Homebrew prefix for dependency lookups (cryptopp, libupnp, libgd,
# gettext, etc.). Same logic as packaging/linux/* but adapted for
# Apple Silicon's /opt/homebrew vs Intel's /usr/local layout.
BREW_PREFIX="$(brew --prefix)"

require_tool() {
    command -v "$1" >/dev/null || {
        echo "fatal: '$1' not on PATH. Install with: $2" >&2
        exit 1
    }
}

build() {
    require_tool cmake "brew install cmake"
    require_tool dylibbundler "brew install dylibbundler"
    require_tool hdiutil "(should be on every macOS, this is unusual)"

    # Match the env-export pattern from build_parameters_all_platforms.md:
    # CPATH/LIBRARY_PATH explicit so cmake/cryptopp.cmake's
    # check_include_file_cxx (which doesn't honour CMAKE_PREFIX_PATH)
    # can find Homebrew headers.
    export CPATH="${BREW_PREFIX}/include"
    export LIBRARY_PATH="${BREW_PREFIX}/lib"
    export PKG_CONFIG_PATH="${BREW_PREFIX}/lib/pkgconfig:$(brew --prefix gd)/lib/pkgconfig:$(brew --prefix libupnp)/lib/pkgconfig:$(brew --prefix gettext)/lib/pkgconfig"
    export PATH="$(brew --prefix gettext)/bin:${PATH}"

    # MACOS_ARCHITECTURES empty → native arch (whatever Homebrew
    # installed for). Setting it to "x86_64;arm64" requires fat
    # libraries everywhere, which standard Homebrew doesn't provide;
    # we hold that for CI's two-runner-then-lipo path.
    local arch_arg=()
    [[ -n "${MACOS_ARCHITECTURES}" ]] && arch_arg+=(-DCMAKE_OSX_ARCHITECTURES="${MACOS_ARCHITECTURES}")

    echo "==> Configuring cmake (target macOS ${MACOS_DEPLOYMENT_TARGET}${MACOS_ARCHITECTURES:+, archs ${MACOS_ARCHITECTURES}})"
    cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}" \
        -DCMAKE_BUILD_TYPE=Release \
        ${arch_arg[@]+"${arch_arg[@]}"} \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET}" \
        -DBUILD_MONOLITHIC=YES \
        -DBUILD_REMOTEGUI=YES \
        -DBUILD_DAEMON=YES \
        -DBUILD_AMULECMD=YES \
        -DBUILD_ED2K=YES \
        -DBUILD_WEBSERVER=YES \
        -DBUILD_TESTING=NO \
        -DENABLE_NLS=YES \
        -DENABLE_UPNP=YES \
        -DENABLE_IP2COUNTRY=YES

    echo "==> Building"
    cmake --build "${BUILD_DIR}" -j"$(sysctl -n hw.ncpu)"

    [[ -d "${APP_BUNDLE}" ]] || { echo "fatal: ${APP_BUNDLE} not produced" >&2; exit 1; }

    echo "==> Bundling dylibs into aMule.app"
    bundle_dylibs "${APP_BUNDLE}"

    if [[ -d "${GUI_BUNDLE}" ]]; then
        echo "==> Bundling dylibs into aMuleGUI.app"
        bundle_dylibs "${GUI_BUNDLE}"
    fi

    # Copy CLI binaries into aMule.app/Contents/MacOS so users have a
    # single .app to install. amule.app/Contents/MacOS/aMule is the
    # GUI entry point; others (amuled, amulecmd, amuleweb, ed2k) are
    # next to it and runnable via `aMule.app/Contents/MacOS/amuled …`
    # from a terminal.
    echo "==> Copying CLI binaries into aMule.app"
    for bin in amuled amulecmd amuleweb ed2k; do
        local src="${BUILD_DIR}/src/${bin}"
        if [[ -x "${src}" ]]; then
            cp -f "${src}" "${APP_BUNDLE}/Contents/MacOS/${bin}"
            # Bundle their dylib deps too — they may pull in libs that
            # aMule itself doesn't (e.g. amulecmd → libreadline).
            dylibbundler -of -b -cd \
                -x "${APP_BUNDLE}/Contents/MacOS/${bin}" \
                -d "${APP_BUNDLE}/Contents/libs/" \
                -p "@executable_path/../libs/" >/dev/null
        fi
    done

    echo "==> Producing .dmg"
    make_dmg
}

bundle_dylibs() {
    local app="$1"
    local main="${app}/Contents/MacOS/$(basename "${app}" .app)"
    [[ -x "${main}" ]] || { echo "fatal: ${main} missing" >&2; exit 1; }

    # -of overwrite, -b copy non-Apple dylibs, -x main exe to inspect,
    # -d destination inside the bundle, -p RPATH prefix for rewrites.
    dylibbundler -of -b -cd \
        -x "${main}" \
        -d "${app}/Contents/libs/" \
        -p "@executable_path/../libs/"
}

make_dmg() {
    local stage="${BUILD_DIR}/dmg-stage"
    rm -rf "${stage}"
    mkdir -p "${stage}"

    # Drag-to-Applications layout: the .app + a symlink to /Applications
    # so users see "drop me here" when they open the .dmg.
    cp -R "${APP_BUNDLE}" "${stage}/"
    [[ -d "${GUI_BUNDLE}" ]] && cp -R "${GUI_BUNDLE}" "${stage}/"
    ln -s /Applications "${stage}/Applications"

    local out="${DIST_DIR}/aMule-${VERSION}-macOS.dmg"
    rm -f "${out}"
    hdiutil create \
        -volname "aMule ${VERSION}" \
        -srcfolder "${stage}" \
        -ov -format UDZO \
        "${out}"

    echo "==> Produced ${out}"
}

sign_and_notarize() {
    "${SCRIPT_DIR}/sign-and-notarize.sh" "${DIST_DIR}/aMule-${VERSION}-macOS.dmg"
}

case "${1:-build}" in
    build) build ;;
    sign)  sign_and_notarize ;;
    *)     usage ;;
esac
