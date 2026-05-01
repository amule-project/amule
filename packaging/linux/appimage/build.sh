#!/bin/bash
# Build the aMule x86_64 / aarch64 AppImage from inside the Dockerfile env.
# Expects the repo bind-mounted at /work and runs from there.

set -euo pipefail

REPO=/work
ARCH=$(uname -m)
# Build tree lives inside the container, not under /work. This keeps each
# run hermetic — cmake's check_cxx_source_compiles cache survives in
# CMakeCache.txt, so a previous run with stale (e.g. pre-libcurl) wx
# would otherwise pin the result of those tests to the broken outcome.
BUILD_DIR=/build-appimage
APPDIR=${BUILD_DIR}/AppDir
ARTIFACT_DIR=${REPO}/dist

# /work is bind-mounted from the host; git refuses to operate on a repo
# owned by a different uid ("dubious ownership"). Whitelist it — this
# container is single-purpose so the safety check buys nothing here.
git config --global --add safe.directory "${REPO}"

VERSION=$(cd "${REPO}" && git describe --tags --always --dirty)

mkdir -p "${ARTIFACT_DIR}"

# 1. Configure + build aMule.
cmake -B "${BUILD_DIR}" -S "${REPO}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_MONOLITHIC=YES \
    -DBUILD_REMOTEGUI=YES \
    -DBUILD_DAEMON=YES \
    -DBUILD_AMULECMD=YES \
    -DBUILD_ED2K=YES \
    -DBUILD_WEBSERVER=YES \
    -DENABLE_NLS=YES \
    -DENABLE_UPNP=YES \
    -DENABLE_IP2COUNTRY=YES

cmake --build "${BUILD_DIR}" -j"$(nproc)"

# 2. Install into the AppDir staging tree at /usr — XDG layout the
#    cmake/install hooks already produce (org.amule.aMule.desktop in
#    share/applications, org.amule.aMule.png in
#    share/icons/hicolor/128x128/apps). Filenames match the canonical
#    AppStream / Wayland app id so the launcher icon binds correctly
#    out of the box (aMule's CamuleApp::OnInit sets g_set_prgname
#    to the same id).
DESTDIR="${APPDIR}" cmake --install "${BUILD_DIR}"

# 3. Bundle every aMule binary, not just amule. The AppImage uses an
#    argv[0]-dispatch AppRun (see ./AppRun) so a symlink named amuled
#    invokes the daemon, amulegui invokes the remote GUI, and so on.
#    Listing each via --executable makes linuxdeploy walk their .so
#    deps and bundle every transitive library.
EXTRA_BINS=(amuled amulegui amulecmd amuleweb ed2k)
EXEC_ARGS=()
for bin in "${EXTRA_BINS[@]}"; do
    if [ -x "${APPDIR}/usr/bin/${bin}" ]; then
        EXEC_ARGS+=(--executable "${APPDIR}/usr/bin/${bin}")
    fi
done

# Force-include libraries that linuxdeploy's AppImage standard
# excludelist drops but that aren't actually present on every distro.
# The Kerberos error-table runtime (libcom_err.so.2) is on the
# excludelist on the assumption it's everywhere — but Debian 13 and
# openSUSE Tumbleweed minimal images don't ship it, breaking the load
# chain libcurl → libgssapi_krb5 → libkrb5 → libcom_err. Bundle it
# explicitly. Same logic may need to extend to other Kerberos
# transitives if future distro drift surfaces them.
LIBARCH_DIR="/usr/lib/$(uname -m)-linux-gnu"
FORCE_LIBS=(libcom_err.so.2)
LIB_ARGS=()
for lib in "${FORCE_LIBS[@]}"; do
    if [ -e "${LIBARCH_DIR}/${lib}" ]; then
        LIB_ARGS+=(--library "${LIBARCH_DIR}/${lib}")
    fi
done

# 4. linuxdeploy bundles wxGTK + GTK3 + every transitive .so via patchelf,
#    rewrites RPATHs, lays out the AppDir.
#    --plugin gtk pulls GTK runtime files (gdk-pixbuf loaders, etc.)
#    that GTK apps need at runtime but aren't picked up by ldd.
#    --custom-apprun supplies the dispatch AppRun (vs linuxdeploy's
#    default which exec's a single fixed binary).
#
#    Run WITHOUT --output appimage so we can post-process the AppDir
#    (strip libreadline) before sealing.
cd "${BUILD_DIR}"
linuxdeploy \
    --appdir "${APPDIR}" \
    --plugin gtk \
    --custom-apprun "${REPO}/packaging/linux/appimage/AppRun" \
    --executable "${APPDIR}/usr/bin/amule" \
    "${EXEC_ARGS[@]}" \
    "${LIB_ARGS[@]}" \
    --desktop-file "${APPDIR}/usr/share/applications/org.amule.aMule.desktop" \
    --icon-file "${APPDIR}/usr/share/icons/hicolor/128x128/apps/org.amule.aMule.png"

# 5. Strip libraries that look like deps but should always be the
#    system version, not a bundled copy. amuled (via wxGetOsDescription)
#    calls popen('/bin/sh -c uname …'); the forked sh inherits our
#    LD_LIBRARY_PATH, finds the bundled libreadline first, and crashes
#    when its symbol versions don't match what the host's sh was linked
#    against (observed on openSUSE Tumbleweed: 'undefined symbol:
#    rl_completion_rewrite_hook'). libreadline + libhistory are paired;
#    libtinfo gets caught in the same dependency net.
# Glob-expansion gotcha: doing this naively (`for so in libreadline.so*;
# do rm -f $APPDIR/usr/lib/$so; done`) globs against CWD, not against
# the AppDir, so the literal pattern reaches rm and silently no-ops.
# cd into the lib dir so the glob expands against real files.
( cd "${APPDIR}/usr/lib" \
    && rm -f libreadline.so* libhistory.so* libtinfo.so* )

# 6. Seal the AppDir into the final AppImage with appimagetool directly.
#    We deliberately do NOT use `linuxdeploy --output appimage` here
#    because it re-walks the binaries' library deps and would re-bundle
#    everything we just stripped. appimagetool is purely a packager —
#    it just packs whatever is in the AppDir.
appimagetool \
    --no-appstream \
    "${APPDIR}" \
    "${BUILD_DIR}/aMule-${VERSION}-${ARCH}.AppImage"

# Move the produced AppImage out to the bind-mounted artifact dir so
# the host can pick it up.
mv aMule-*-*.AppImage "${ARTIFACT_DIR}/"

# The container runs as root; without this, ${ARTIFACT_DIR} and the
# .AppImage end up root-owned on the host and the invoking user can't
# manage / sign / sym-link them. HOST_UID/HOST_GID are passed in by
# packaging/linux/build.sh via -e so we don't have to guess.
if [ -n "${HOST_UID:-}" ] && [ -n "${HOST_GID:-}" ]; then
    chown -R "${HOST_UID}:${HOST_GID}" "${ARTIFACT_DIR}"
fi

echo "==> Produced ${ARTIFACT_DIR}/$(basename ${ARTIFACT_DIR}/aMule-*-*.AppImage)"
