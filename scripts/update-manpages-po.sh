#! /bin/bash

# Regenerates docs/man/po/manpages.pot from the English masters and merges
# the result into every docs/man/po/manpages-*.po. Counterpart to
# scripts/update-po.sh for the application catalogs.
#
# Run after editing any English manpage master (docs/man/*.1.in or
# src/utils/*/docs/*.1.in) and commit the diff.

die () {
	local ERR_CODE=$?
	[[ $ERR_CODE == 0 ]] && return

	local EXIT_CODE=$1
	shift
	local MESSAGE=( "$@" )

	[[ ${MESSAGE[*]} != "" ]] && echo "${MESSAGE[@]}" >&2
	exit "${EXIT_CODE}"
}

usage() {
	echo "Regenerates docs/man/po/manpages.pot from the English manpage masters"
	echo "and merges it into every docs/man/po/manpages-*.po"
	echo
	echo "Usage: $0 [-h | --help]"
	echo "  -h"
	echo "  --help       Display this help message"
}

if ! PARAMS=$(getopt -o "h" -l "help" -n "$0" -- "$@"); then
	usage
	false; die 10
fi

eval set -- "$PARAMS"

while true; do
	case "$1" in
	-h | --help )
		usage
		false; die 0
		;;
	-- )
		shift
		break
		;;
	* )
		usage
		false; die 12 "Error processing command line parameters"
		;;
	esac
done

GIT_ROOT=$(git rev-parse --show-toplevel)
[[ ${PWD} == "${GIT_ROOT}" ]]
die 12 \
	" The current path is: '${PWD}'" \
	$'\n' \
	"This script must be run in '${GIT_ROOT}'"

if ! command -v po4a &>/dev/null; then
	echo "Error: po4a not found. Install po4a (https://po4a.org)." >&2
	exit 20
fi

if ! command -v msgmerge &>/dev/null; then
	echo "Error: msgmerge not found. Install gettext." >&2
	exit 21
fi

# The build-time po4a.config.in uses absolute paths so po4a can land its
# rendered outputs in the build dir. For pot-only updates we want
# docs/man-relative paths instead -- otherwise po4a stamps every #: source
# reference in manpages.pot with the developer's absolute file path, and
# the commit becomes machine-specific.
#
# Materialise a temp config with the CMake variable refs collapsed back to
# what they used to be in the legacy hand-written po4a.config (docs/man as
# the working directory), run po4a from there with --no-translations to
# skip rendering, and clean up.
TMP_CONFIG=$(mktemp -t amule-po4a-config.XXXXXX)
trap 'rm -f "${TMP_CONFIG}"' EXIT

sed \
	-e "s|@CMAKE_CURRENT_SOURCE_DIR@/||g" \
	-e "s|@CMAKE_CURRENT_BINARY_DIR@/||g" \
	-e "s|@CMAKE_SOURCE_DIR@|../..|g" \
	-e "s|@CMAKE_BINARY_DIR@|../..|g" \
	"${GIT_ROOT}/docs/man/po4a.config.in" > "${TMP_CONFIG}"
die 30 "failed to materialise temporary po4a.config"

cd "${GIT_ROOT}/docs/man"
echo "Updating po/manpages.pot and merging into po/manpages-*.po ..."
# --no-translations: skip rendering of *.{lang}.1.in -- those are built
# from CMake at build time, not from this script.
# --force: regenerate the .pot even when po4a thinks it's up to date.
po4a --no-translations --force "${TMP_CONFIG}"
die 31 "po4a failed"

echo "Done."
exit 0
