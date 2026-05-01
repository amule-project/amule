#!/bin/bash
# Single entry point for Linux packaging.
# Reads packaging/linux/versions.env (the only file you ever hand-edit)
# and produces the requested artifact.
#
# Usage:
#   packaging/linux/build.sh appimage [arch]   # arch = host (default), x86_64, aarch64
#   packaging/linux/build.sh flatpak           # render manifest + flatpak-builder

set -euo pipefail

usage() {
    cat >&2 <<EOF
usage: $0 <command> [args]

commands:
  appimage [arch]            produce .AppImage in dist/
                             arch: host (default) | x86_64 | aarch64
  flatpak [arch]             produce .flatpak bundle in dist/
                             arch: host (default) | x86_64 | aarch64
  validate [arch]            run amuled --version inside a fixed matrix
                             of distro Docker images to catch lib-load
                             regressions. arch: host (default) only —
                             cross-arch validation isn't possible
                             (AppImage runtime fails under QEMU).
  render-flatpak-manifest    just render the manifest, don't build
  all                        produce all 4 artifacts (appimage+flatpak,
                             both arches). Cross-arch needs setup.
  setup-cross-arch           one-time: install tonistiigi/binfmt for
                             docker cross-arch + Flatpak runtimes for
                             both arches. Requires --privileged docker.

Cross-arch builds (target arch != host arch) need 'setup-cross-arch'
to have been run once on this host. Native builds (target == host)
work without setup. Cross-arch is ~5-10× slower (QEMU emulation).
EOF
    exit 64
}

[[ $# -ge 1 ]] || usage

# Resolve repo root and packaging dir regardless of CWD.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
VERSIONS_ENV="${SCRIPT_DIR}/versions.env"

[[ -f "${VERSIONS_ENV}" ]] || {
    echo "fatal: ${VERSIONS_ENV} not found" >&2
    exit 1
}

# shellcheck disable=SC1090
set -a; source "${VERSIONS_ENV}"; set +a

target="$1"; shift || true

build_appimage() {
    # Resolve target arch. "host" (default) builds natively for the
    # current machine; x86_64/aarch64 build via QEMU emulation if not
    # native (slow — wx-from-source step takes ~30-50 min on aarch64-
    # emulating-x86_64 vs ~5 min native).
    local arch_in="${1:-host}"
    local target_arch
    case "${arch_in}" in
        host)    target_arch="$(uname -m)" ;;
        x86_64)  target_arch=x86_64 ;;
        aarch64) target_arch=aarch64 ;;
        *)       echo "fatal: unsupported arch '${arch_in}' (choose host|x86_64|aarch64)" >&2; exit 1 ;;
    esac

    # Map uname -m form -> Docker --platform form.
    local docker_platform
    case "${target_arch}" in
        x86_64)  docker_platform=linux/amd64 ;;
        aarch64) docker_platform=linux/arm64 ;;
    esac

    local image_tag="amule-appimage:${UBUNTU_BASE}-${target_arch}"
    echo "==> Building Docker image ${image_tag} (platform=${docker_platform})"

    # Pre-flight: cross-arch builds need tonistiigi/binfmt registered.
    # Run 'setup-cross-arch' once if missing — friendlier than letting
    # docker build fail mid-way with 'exec format error'.
    if [ "${target_arch}" != "$(uname -m)" ]; then
        local probe_image="alpine:latest"
        if ! docker run --rm --platform "${docker_platform}" \
                "${probe_image}" /bin/true 2>/dev/null; then
            echo "fatal: cross-arch build requested (target=${target_arch}, host=$(uname -m)) but binfmt not registered." >&2
            echo "       run once on this host: $0 setup-cross-arch" >&2
            exit 1
        fi
    fi
    docker build \
        --platform "${docker_platform}" \
        --build-arg "UBUNTU_BASE=${UBUNTU_BASE}" \
        --build-arg "WX_VERSION=${WX_VERSION}" \
        --build-arg "WX_SHA256=${WX_SHA256}" \
        --build-arg "LINUXDEPLOY_VERSION=${LINUXDEPLOY_VERSION}" \
        --build-arg "LINUXDEPLOY_GTK_SHA=${LINUXDEPLOY_GTK_SHA}" \
        -t "${image_tag}" \
        "${SCRIPT_DIR}/appimage"

    echo "==> Building AppImage (${target_arch})"
    # HOST_UID/HOST_GID let the in-container script chown the produced
    # artifact back to the invoking user — Docker would otherwise leave
    # dist/ owned by root.
    docker run --rm \
        --platform "${docker_platform}" \
        --device /dev/fuse \
        --cap-add SYS_ADMIN \
        --security-opt apparmor:unconfined \
        -e "HOST_UID=$(id -u)" \
        -e "HOST_GID=$(id -g)" \
        -v "${REPO_ROOT}:/work" \
        "${image_tag}"
}

# Render <APP_ID>.yaml.in → <APP_ID>.yaml.
# Done as a discrete subcommand so CI / Flathub can re-render without
# building. envsubst is part of gettext-base, present on every target.
render_flatpak_manifest() {
    local in="${SCRIPT_DIR}/flatpak/${APP_ID}.yaml.in"
    local out="${SCRIPT_DIR}/flatpak/${APP_ID}.yaml"
    [[ -f "${in}" ]] || {
        echo "fatal: ${in} not found" >&2
        exit 1
    }
    command -v envsubst >/dev/null || {
        echo "fatal: envsubst missing — install 'gettext-base' (Debian/Ubuntu) or 'gettext' (others)" >&2
        exit 1
    }
    echo "==> Rendering ${out}"
    envsubst < "${in}" > "${out}"
}

build_flatpak() {
    # Resolve target arch (same shape as build_appimage).
    local arch_in="${1:-host}"
    local target_arch
    case "${arch_in}" in
        host)    target_arch="$(uname -m)" ;;
        x86_64)  target_arch=x86_64 ;;
        aarch64) target_arch=aarch64 ;;
        *)       echo "fatal: unsupported arch '${arch_in}' (choose host|x86_64|aarch64)" >&2; exit 1 ;;
    esac

    # Pre-flight: cross-arch needs the GNOME runtime+SDK installed for
    # the target arch, and binfmt for the QEMU shim flatpak-builder
    # invokes. setup-cross-arch handles both.
    if [ "${target_arch}" != "$(uname -m)" ]; then
        if ! flatpak info --user --arch "${target_arch}" "org.gnome.Platform//${GNOME_RUNTIME_VERSION}" >/dev/null 2>&1; then
            echo "fatal: cross-arch flatpak build requested (target=${target_arch}) but org.gnome.Platform//${GNOME_RUNTIME_VERSION} not installed for that arch." >&2
            echo "       run once on this host: $0 setup-cross-arch" >&2
            exit 1
        fi
    fi

    render_flatpak_manifest
    local manifest="${SCRIPT_DIR}/flatpak/${APP_ID}.yaml"
    local statedir="${REPO_ROOT}/build-flatpak-${target_arch}"
    local repodir="${REPO_ROOT}/build-flatpak-${target_arch}-repo"
    local artifact_dir="${REPO_ROOT}/dist"
    mkdir -p "${artifact_dir}"

    # Cap module-build parallelism to half the host's CPU count.
    # Wx and aMule have C++ TUs that peak at ~1-2 GB RAM each; on
    # smaller VMs (≤8 GB RAM) full -j$(nproc) reliably OOM-kills the
    # build mid-way. Override per-host with FLATPAK_MAX_JOBS=N.
    local jobs="${FLATPAK_MAX_JOBS:-$(( $(nproc) / 2 ))}"
    [ "${jobs}" -lt 1 ] && jobs=1

    echo "==> flatpak-builder --arch=${target_arch} ${manifest} (FLATPAK_BUILDER_N_JOBS=${jobs})"
    cd "${SCRIPT_DIR}/flatpak"
    FLATPAK_BUILDER_N_JOBS="${jobs}" flatpak-builder --user --force-clean \
        --arch="${target_arch}" \
        --repo="${repodir}" \
        "${statedir}" "${manifest}"

    # Bundle to a single .flatpak file in dist/. Includes the version
    # string + arch in the filename so dist/ ends up with one bundle
    # per (arch, version) combo, parallel to the AppImage layout.
    local version
    version=$(cd "${REPO_ROOT}" && git describe --tags --always --dirty 2>/dev/null || echo "snapshot")
    local bundle="${artifact_dir}/aMule-${version}-${target_arch}.flatpak"
    echo "==> Bundling ${bundle}"
    flatpak build-bundle --arch="${target_arch}" \
        "${repodir}" "${bundle}" "${APP_ID}" "${AMULE_REVISION}"

    echo "==> Done: ${bundle}"
}

setup_cross_arch() {
    # tonistiigi/binfmt registers QEMU emulators in the host kernel's
    # binfmt_misc so docker can run / build images for non-native
    # arches. Privileged because it writes to /proc/sys/fs/binfmt_misc.
    # One-time per host (registration survives reboot).
    echo "==> Registering QEMU binfmt handlers for docker (privileged)"
    docker run --rm --privileged tonistiigi/binfmt --install all
    echo
    echo "==> Verifying docker x86_64 + aarch64 emulation works"
    for plat in linux/amd64 linux/arm64; do
        printf "  %s: " "${plat}"
        docker run --rm --platform "${plat}" alpine uname -m 2>&1 | tail -1
    done
    echo

    # Flatpak needs the GNOME runtime + SDK installed for both arches
    # (the host arch is presumably already there; install the other).
    # flatpak-builder's QEMU shim picks up the same binfmt_misc handlers
    # registered by tonistiigi/binfmt above.
    if command -v flatpak >/dev/null; then
        local host_arch other_arch
        host_arch="$(uname -m)"
        case "${host_arch}" in
            x86_64)  other_arch=aarch64 ;;
            aarch64) other_arch=x86_64 ;;
            *)       other_arch="" ;;
        esac

        echo "==> Installing GNOME Platform + SDK ${GNOME_RUNTIME_VERSION} for both arches"
        flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
        flatpak install --user --noninteractive --arch "${host_arch}" \
            flathub "org.gnome.Platform//${GNOME_RUNTIME_VERSION}" "org.gnome.Sdk//${GNOME_RUNTIME_VERSION}"
        if [ -n "${other_arch}" ]; then
            flatpak install --user --noninteractive --arch "${other_arch}" \
                flathub "org.gnome.Platform//${GNOME_RUNTIME_VERSION}" "org.gnome.Sdk//${GNOME_RUNTIME_VERSION}"
        fi
    else
        echo "==> Skipping Flatpak runtime install (flatpak not installed on host)"
    fi
}

build_all() {
    echo "==> Building all 4 artifacts: appimage+flatpak × x86_64+aarch64"
    build_appimage x86_64
    build_appimage aarch64
    build_flatpak x86_64
    build_flatpak aarch64
    echo
    echo "==> All artifacts in ${REPO_ROOT}/dist/"
    ls -la "${REPO_ROOT}/dist/"
}

# Cross-distro Docker validation: runs amuled --version inside a fixed
# matrix of distro images, exercising the AppImage's runtime against
# different glibc / wxGTK / system-lib combinations. Catches the kind
# of drift that bit us during phase 1 (libcom_err on Debian 13 +
# openSUSE Tumbleweed, libreadline ABI clash on Tumbleweed).
#
# Usage: packaging/linux/build.sh validate [host|x86_64|aarch64]
#
# Note: cross-arch validation does NOT work — the AppImage type-2
# runtime ELF doesn't execute under QEMU user-mode emulation (same
# limitation that forced unsquashfs-based extraction in the build
# Dockerfile). validate exits with a useful message in that case.
validate_appimage() {
    local arch_in="${1:-host}"
    local target_arch
    case "${arch_in}" in
        host)    target_arch="$(uname -m)" ;;
        x86_64)  target_arch=x86_64 ;;
        aarch64) target_arch=aarch64 ;;
        *) echo "fatal: arch '${arch_in}' (choose host|x86_64|aarch64)" >&2; exit 1 ;;
    esac

    if [ "${target_arch}" != "$(uname -m)" ]; then
        echo "fatal: cross-arch AppImage validation isn't supported." >&2
        echo "       The AppImage type-2 runtime can't execute under QEMU user-mode" >&2
        echo "       emulation; validation needs a real ${target_arch} host (or CI runner)." >&2
        exit 1
    fi

    # Find the matching AppImage in dist/ — accepts the version-bearing
    # filename produced by build_appimage. -t sorts by mtime newest-first
    # so iterating builds in the same dist/ always validates the latest.
    local appimage
    appimage=$(ls -t "${REPO_ROOT}/dist/aMule-"*"-${target_arch}.AppImage" 2>/dev/null | head -1)
    [ -n "${appimage}" ] || {
        echo "fatal: no AppImage found in ${REPO_ROOT}/dist/ matching arch ${target_arch}" >&2
        echo "       run 'packaging/linux/build.sh appimage ${arch_in}' first" >&2
        exit 1
    }

    echo "==> Validating $(basename "${appimage}")"
    echo

    # Stage at a fixed path so each container can mount it read-only.
    local stage=/tmp/amule-validate-${target_arch}.AppImage
    cp -f "${appimage}" "${stage}"
    chmod +x "${stage}"

    local -A results
    local distro
    local docker_platform
    case "${target_arch}" in
        x86_64)  docker_platform=linux/amd64 ;;
        aarch64) docker_platform=linux/arm64 ;;
    esac

    local distros=(
        "ubuntu:22.04"
        "ubuntu:24.04"
        "debian:12"
        "debian:13"
        "fedora:40"
        "fedora:42"
        "opensuse/tumbleweed"
    )

    for distro in "${distros[@]}"; do
        printf "  %-25s " "${distro}"
        local out
        out=$(docker run --rm --platform "${docker_platform}" \
            --device /dev/fuse --cap-add SYS_ADMIN \
            --security-opt apparmor:unconfined \
            -v "${stage}:/test.AppImage:ro" \
            --entrypoint /bin/sh \
            "${distro}" \
            -c "ln -sf /test.AppImage /amuled && /amuled --version 2>/dev/null" 2>/dev/null || true)
        out=$(echo "${out}" | grep -E "^aMuleD" | head -1)
        if [ -n "${out}" ]; then
            results[${distro}]=PASS
            echo "✅ ${out}"
        else
            results[${distro}]=FAIL
            echo "❌"
        fi
    done

    echo
    echo "==> Summary"
    local pass=0 fail=0
    for distro in "${distros[@]}"; do
        case "${results[${distro}]:-FAIL}" in
            PASS) pass=$((pass+1)) ;;
            *)    fail=$((fail+1)) ;;
        esac
    done
    echo "    ${pass}/${#distros[@]} passed"

    rm -f "${stage}"
    [ "${fail}" -eq 0 ] || exit 1
}

case "${target}" in
    appimage)                build_appimage "${1:-}" ;;
    flatpak)                 build_flatpak "${1:-}" ;;
    render-flatpak-manifest) render_flatpak_manifest ;;
    validate)                validate_appimage "${1:-}" ;;
    all)                     build_all ;;
    setup-cross-arch)        setup_cross_arch ;;
    *)                       usage ;;
esac
