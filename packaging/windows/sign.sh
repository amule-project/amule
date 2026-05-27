#!/bin/bash
# signtool the produced Windows artifact — either the portable .zip
# (extract → sign all .exe + .dll inside → re-zip) or the installer
# .exe (sign in place). Target kind is detected from the file extension.
#
# No-op when WIN_CERT_* env vars aren't set — keeps the v1 release
# flow unsigned-but-shippable while letting CI flip the switch later
# by adding repo secrets.
#
# Required env (all must be set; otherwise this script exits 0 silently):
#   WIN_CERT_PFX_BASE64   base64-encoded .pfx (cert + private key)
#   WIN_CERT_PASSWORD     .pfx password

set -euo pipefail

target="${1:?usage: $0 <path-to-.zip-or-.exe>}"
[[ -e "${target}" ]] || { echo "fatal: ${target} doesn't exist" >&2; exit 1; }

# Skip silently when any required secret is missing — that's the
# "unsigned v1" path.
for var in WIN_CERT_PFX_BASE64 WIN_CERT_PASSWORD; do
    if [[ -z "${!var:-}" ]]; then
        echo "==> ${var} not set, skipping signing (v1 unsigned path)"
        exit 0
    fi
done

require_tool() {
    command -v "$1" >/dev/null || {
        echo "fatal: '$1' not on PATH" >&2
        exit 1
    }
}
require_tool signtool "(part of Windows SDK; install via Visual Studio Build Tools)"

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

# Decode the .pfx
PFX="${WORK}/cert.pfx"
echo "${WIN_CERT_PFX_BASE64}" | base64 --decode > "${PFX}"

case "${target}" in
    *.exe)
        # Installer .exe — sign in place. signtool returns 0 even on
        # already-signed binaries (re-signs).
        echo "==> Signing ${target}"
        signtool sign \
            /f "${PFX}" /p "${WIN_CERT_PASSWORD}" \
            /tr http://timestamp.digicert.com /td sha256 \
            /fd sha256 /v "${target}"
        echo "==> Verifying signature"
        signtool verify /pa /v "${target}" || true
        echo "==> Signed: ${target}"
        ;;
    *.zip)
        require_tool zip "pacman -S zip"
        # Extract the zip, sign every .exe + .dll inside, re-zip.
        echo "==> Extracting ${target}"
        unzip -q "${target}" -d "${WORK}/extract"

        echo "==> Signing all .exe and .dll inside"
        find "${WORK}/extract" -type f \( -name '*.exe' -o -name '*.dll' \) -print0 \
            | xargs -0 -n 50 signtool sign \
                /f "${PFX}" /p "${WIN_CERT_PASSWORD}" \
                /tr http://timestamp.digicert.com /td sha256 \
                /fd sha256 /v

        echo "==> Re-zipping"
        cd "${WORK}/extract"
        rm -f "${target}"
        zip -rqX "${target}" .

        echo "==> Verifying signature on amule.exe"
        signtool verify /pa /v "${WORK}/extract"/*/bin/amule.exe || true

        echo "==> Signed: ${target}"
        ;;
    *)
        echo "fatal: ${target} — unsupported extension (need .zip or .exe)" >&2
        exit 1
        ;;
esac
