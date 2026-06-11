#! /bin/bash
# Headless smoke test for the IP2Country source-to-URL resolver. Boots
# amuled once per source configuration, captures the "Download new ..."
# line from the log, and asserts the URL matches the expected pattern.
#
# Run from the repository root with a built amuled binary at
# build/src/amuled (or pass AMULED=<path> as an environment variable).

set -e

AMULED=${AMULED:-build/src/amuled}
if [[ ! -x "${AMULED}" ]]; then
	echo "Error: ${AMULED} not built. Run cmake --build build --target amuled first." >&2
	exit 2
fi

TMPDIR=$(mktemp -d -t amule-geoip-test.XXXXXX)
trap 'rm -rf "${TMPDIR}"; pkill -P $$ 2>/dev/null || true' EXIT

# Disabling the master enable means amuled never actually runs the
# download — we only want to observe the URL the resolver builds. We
# inject the URL into the log by enabling AutoUpdate and capturing the
# "Download new ..." line that StartDownload() emits before reaching
# the network.

pass=0
fail=0

run_case() {
	local label=$1
	local conf=$2
	local expected_regex=$3

	local cfg="${TMPDIR}/${label}"
	mkdir -p "${cfg}/.aMule"
	cat > "${cfg}/.aMule/amule.conf" <<EOF
[eMule]
GeoIPEnabled=1
GeoIPAutoUpdate=1
ECPassword=d41d8cd98f00b204e9800998ecf8427e
AcceptExternalConnections=1
${conf}
EOF

	HOME="${cfg}" "${AMULED}" --config-dir="${cfg}/.aMule" -o &> "${cfg}/out.log" &
	local pid=$!
	# Wait briefly for amuled to initialise + try its first update.
	sleep 4
	kill ${pid} 2>/dev/null || true
	wait ${pid} 2>/dev/null || true

	local url
	url=$(grep -oE "Download new [^ ]+ from .*" "${cfg}/out.log" | tail -1 | sed 's|.* from ||')

	if [[ -z "${url}" ]]; then
		echo "[${label}] FAIL — no Download-URL log line found"
		echo "  last 5 log lines:"
		tail -5 "${cfg}/out.log" | sed 's|^|    |'
		fail=$((fail+1))
		return
	fi

	if [[ "${url}" =~ ${expected_regex} ]]; then
		echo "[${label}] PASS — ${url}"
		pass=$((pass+1))
	else
		echo "[${label}] FAIL — got '${url}', expected to match /${expected_regex}/"
		fail=$((fail+1))
	fi
}

# DB-IP: expect current-month URL template.
this_year=$(date -u +%Y)
this_month=$(date -u +%m)
run_case "dbip" \
	"GeoIPSource=dbip" \
	"^https://download\\.db-ip\\.com/free/dbip-country-lite-${this_year}-${this_month}\\.mmdb\\.gz$"

# MaxMind: expect license key in the query-string download URL. The key
# below is obviously fake — the test only checks URL composition.
run_case "maxmind" \
	"GeoIPSource=maxmind
GeoIPMaxMindLicense=test-key" \
	"^https://download\\.maxmind\\.com/app/geoip_download\\?edition_id=GeoLite2-Country&license_key=test-key&suffix=tar\\.gz$"

# Custom: expect URL passed through verbatim.
run_case "custom" \
	"GeoIPSource=custom
GeoIPCustomUrl=https://example.org/path/geoip.mmdb.gz" \
	"^https://example\\.org/path/geoip\\.mmdb\\.gz$"

# Empty MaxMind credentials: expect StartDownload to bail with the
# "no Account ID / License Key configured" log line and never emit a
# Download-new line.
mkdir -p "${TMPDIR}/maxmind-empty/.aMule"
cat > "${TMPDIR}/maxmind-empty/.aMule/amule.conf" <<EOF
[eMule]
GeoIPEnabled=1
GeoIPAutoUpdate=1
GeoIPSource=maxmind
ECPassword=d41d8cd98f00b204e9800998ecf8427e
AcceptExternalConnections=1
EOF
HOME="${TMPDIR}/maxmind-empty" "${AMULED}" --config-dir="${TMPDIR}/maxmind-empty/.aMule" -o &> "${TMPDIR}/maxmind-empty/out.log" &
mempid=$!
sleep 4
kill ${mempid} 2>/dev/null || true
wait ${mempid} 2>/dev/null || true

if grep -q "MaxMind selected as the GeoIP source but no License Key" "${TMPDIR}/maxmind-empty/out.log"; then
	echo "[maxmind-empty] PASS — surfaces missing-credentials error"
	pass=$((pass+1))
else
	echo "[maxmind-empty] FAIL — expected missing-credentials log line"
	fail=$((fail+1))
fi

echo
echo "----- summary: ${pass} pass / ${fail} fail -----"
exit $((fail > 0))
