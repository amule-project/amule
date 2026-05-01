#!/bin/bash
# codesign + notarize a macOS .dmg / .app.
#
# No-op when APPLE_* env vars aren't set — keeps the v1 release flow
# unsigned-but-shippable while letting CI flip the switch later by
# adding repo secrets.
#
# Required env (all must be set; otherwise this script exits 0 silently):
#   APPLE_DEVELOPER_ID    "Developer ID Application: Name (TEAMID)"
#   APPLE_TEAM_ID         10-character team identifier
#   APPLE_NOTARY_USER     Apple ID email
#   APPLE_NOTARY_PASS     app-specific password for notarytool
#   APPLE_CERT_P12_BASE64 base64-encoded .p12 (cert + private key)
#   APPLE_CERT_PASSWORD   .p12 password

set -euo pipefail

target="${1:?usage: $0 <path-to-.dmg-or-.app>}"
[[ -e "${target}" ]] || { echo "fatal: ${target} doesn't exist" >&2; exit 1; }

# Skip silently when any required secret is missing — that's the
# "unsigned v1" path. Each missing var produces no output to keep
# normal builds quiet.
for var in APPLE_DEVELOPER_ID APPLE_TEAM_ID APPLE_NOTARY_USER APPLE_NOTARY_PASS APPLE_CERT_P12_BASE64 APPLE_CERT_PASSWORD; do
    if [[ -z "${!var:-}" ]]; then
        echo "==> ${var} not set, skipping signing (v1 unsigned path)"
        exit 0
    fi
done

# Decode the .p12 and import into a temporary keychain so we don't
# pollute the user's login keychain (and so CI runners can do this
# repeatably).
KEYCHAIN="${HOME}/Library/Keychains/amule-build-tmp.keychain-db"
KEYCHAIN_PW="$(openssl rand -hex 16)"
P12_PATH="$(mktemp -t amule-p12-XXXXXX).p12"

cleanup() {
    security delete-keychain "${KEYCHAIN}" 2>/dev/null || true
    rm -f "${P12_PATH}"
}
trap cleanup EXIT

echo "==> Importing signing cert into temp keychain"
echo "${APPLE_CERT_P12_BASE64}" | base64 --decode > "${P12_PATH}"
security create-keychain -p "${KEYCHAIN_PW}" "${KEYCHAIN}"
security set-keychain-settings -lut 7200 "${KEYCHAIN}"
security unlock-keychain -p "${KEYCHAIN_PW}" "${KEYCHAIN}"
security import "${P12_PATH}" -k "${KEYCHAIN}" -P "${APPLE_CERT_PASSWORD}" \
    -T /usr/bin/codesign
security set-key-partition-list -S apple-tool:,apple:,codesign: \
    -s -k "${KEYCHAIN_PW}" "${KEYCHAIN}" >/dev/null
security list-keychains -d user -s "${KEYCHAIN}" \
    $(security list-keychains -d user | tr -d '"')

# If target is a .dmg, sign the .app inside first, then sign the .dmg.
# If target is a .app directly, just sign it.
if [[ "${target}" == *.dmg ]]; then
    MOUNT="$(mktemp -d)"
    hdiutil attach -nobrowse -mountpoint "${MOUNT}" "${target}"
    for app in "${MOUNT}"/*.app; do
        echo "==> Signing $(basename "${app}")"
        codesign --deep --force --options runtime --timestamp \
            --sign "${APPLE_DEVELOPER_ID}" \
            --keychain "${KEYCHAIN}" \
            "${app}"
        codesign --verify --strict --verbose=2 "${app}"
    done
    hdiutil detach "${MOUNT}"

    echo "==> Signing .dmg"
    codesign --force --timestamp \
        --sign "${APPLE_DEVELOPER_ID}" \
        --keychain "${KEYCHAIN}" \
        "${target}"
elif [[ "${target}" == *.app ]]; then
    echo "==> Signing $(basename "${target}")"
    codesign --deep --force --options runtime --timestamp \
        --sign "${APPLE_DEVELOPER_ID}" \
        --keychain "${KEYCHAIN}" \
        "${target}"
    codesign --verify --strict --verbose=2 "${target}"
else
    echo "fatal: ${target} is neither .dmg nor .app" >&2
    exit 1
fi

echo "==> Submitting to Apple notary service"
xcrun notarytool submit "${target}" \
    --apple-id "${APPLE_NOTARY_USER}" \
    --password "${APPLE_NOTARY_PASS}" \
    --team-id "${APPLE_TEAM_ID}" \
    --wait

echo "==> Stapling notarization ticket"
xcrun stapler staple "${target}"
xcrun stapler validate "${target}"

echo "==> Signed + notarized: ${target}"
