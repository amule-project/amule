# Build args sourced from packaging/linux/versions.env via build.sh.
# Defining ARGs without defaults makes the build fail loud if any pin
# is missing — caller must source versions.env.
ARG UBUNTU_BASE

FROM ubuntu:${UBUNTU_BASE}

ARG WX_VERSION
ARG WX_SHA256
ARG LINUXDEPLOY_VERSION
ARG LINUXDEPLOY_GTK_SHA

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        git \
        ca-certificates \
        wget \
        file \
        fuse \
        # aMule build deps (matches CI Ubuntu job).
        # wxWidgets is built from source below — Ubuntu 22.04 doesn't
        # package wx 3.2.x, and bumping the base to 24.04 would push
        # the glibc floor to 2.39 and drop Ubuntu 22.04 LTS users.
        binutils-dev \
        gettext \
        libboost-dev \
        libcrypto++-dev \
        libgd-dev \
        libgeoip-dev \
        # libmaxminddb: enables IP2Country (country flag lookup) when
        # the user supplies their own GeoLite2-Country.mmdb at runtime.
        # We don't ship the database — MaxMind's licence forbids
        # redistribution; users drop it in ~/.aMule/.
        libmaxminddb-dev \
        libreadline-dev \
        libupnp-dev \
        # libcurl: backend for wxWebRequest, which aMule's HTTP path requires.
        libcurl4-openssl-dev \
        # GTK3 dev headers (linker target for wx) + Wayland/X11 client headers
        libgtk-3-dev \
        libwayland-dev \
        # libayatana-appindicator3: enables the StatusNotifierItem (SNI)
        # tray-icon backend in MuleTrayIcon. Without it the tray falls
        # back to wxTaskBarIcon → legacy GtkStatusIcon, which GNOME
        # dropped in 3.26 and wlroots compositors never implemented.
        # Ubuntu's GNOME Shell extension renders both, but Fedora /
        # vanilla GNOME / Sway need the SNI path to see the icon at all.
        libayatana-appindicator3-dev \
        # AppImage runtime / linuxdeploy-plugin-gtk deps
        libglib2.0-bin \
        libgdk-pixbuf-2.0-0 \
        librsvg2-common \
        gtk-update-icon-cache \
        desktop-file-utils \
        # zsync delta-update tool used by appimagetool
        zsync \
        # unsquashfs to extract AppImage type-2 contents directly,
        # bypassing the AppImage runtime which doesn't run under
        # QEMU user-mode emulation when cross-building.
        squashfs-tools \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# wxWidgets from source. --disable-shared so wx links statically into
# aMule, sidestepping the wxGTK-SONAME-mismatch problem AppImage users
# hit on distros shipping a different wx 3.2 patch level than the
# Ubuntu 22.04 base. Subsystems aMule doesn't use are disabled to
# shrink the static archives.
WORKDIR /opt/wxwidgets
RUN wget -q "https://github.com/wxWidgets/wxWidgets/releases/download/v${WX_VERSION}/wxWidgets-${WX_VERSION}.tar.bz2" -O wx.tar.bz2 \
    && echo "${WX_SHA256}  wx.tar.bz2" | sha256sum -c - \
    && tar xf wx.tar.bz2 \
    && cd "wxWidgets-${WX_VERSION}" \
    && ./configure \
        --prefix=/usr/local \
        --with-gtk=3 \
        --enable-unicode \
        --disable-debug \
        --disable-shared \
        --disable-mediactrl \
        --disable-webview \
        --disable-richtext \
        --disable-aui \
        --disable-html \
        --without-libtiff \
        --without-libjbig \
        --without-libmspack \
        --without-opengl \
        --with-libcurl \
    && make -j"$(nproc)" \
    && make install \
    && cd / \
    && rm -rf /opt/wxwidgets

# linuxdeploy is fetched from a tagged Release (alpha tags get pruned;
# bump LINUXDEPLOY_VERSION when an old pin 404s).
# linuxdeploy-plugin-gtk has no Releases — fetched from a pinned commit
# on master via raw.githubusercontent.com.
WORKDIR /opt/linuxdeploy
# Helper: find the squashfs offset in an AppImage and unsquashfs it.
# We can't use `./X.AppImage --appimage-extract` here because the
# AppImage runtime ELF doesn't execute under QEMU user-mode emulation
# (some FUSE-related syscalls are unsupported), and we need this to
# work both natively and when cross-building.
#
# AppImage type-2 = small ELF runtime + appended squashfs. The
# squashfs starts at some offset, marked by the 'hsqs' magic at its
# beginning. The ELF can also contain false 'hsqs' substrings, so
# iterate over candidates and take the first one whose superblock
# parses cleanly.
COPY <<'EOF' /usr/local/bin/extract-appimage
#!/bin/bash
set -euo pipefail
src="$1"; dst="$2"
for offset in $(grep -aob 'hsqs' "${src}" | cut -d: -f1); do
    if unsquashfs -q -o "${offset}" -d "${dst}" "${src}" >/dev/null 2>&1; then
        exit 0
    fi
done
echo "extract-appimage: no valid squashfs found in ${src}" >&2
exit 1
EOF
RUN chmod +x /usr/local/bin/extract-appimage \
    && ARCH=$(uname -m) \
    && wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/${LINUXDEPLOY_VERSION}/linuxdeploy-${ARCH}.AppImage" -O linuxdeploy.AppImage \
    && wget -q "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/${LINUXDEPLOY_GTK_SHA}/linuxdeploy-plugin-gtk.sh" -O linuxdeploy-plugin-gtk.sh \
    && wget -q "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${ARCH}.AppImage" -O appimagetool.AppImage \
    && chmod +x linuxdeploy-plugin-gtk.sh \
    && extract-appimage linuxdeploy.AppImage linuxdeploy-extracted \
    && extract-appimage appimagetool.AppImage appimagetool-extracted \
    && ln -s /opt/linuxdeploy/linuxdeploy-extracted/AppRun /usr/local/bin/linuxdeploy \
    && ln -s /opt/linuxdeploy/linuxdeploy-plugin-gtk.sh /usr/local/bin/linuxdeploy-plugin-gtk \
    && ln -s /opt/linuxdeploy/appimagetool-extracted/AppRun /usr/local/bin/appimagetool

WORKDIR /work
ENTRYPOINT ["/work/packaging/linux/appimage/build.sh"]
