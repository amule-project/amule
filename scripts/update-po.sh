#! /bin/bash

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
	echo "Regenerates po/amule.pot from source and merges it into all po/*.po files"
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

if ! command -v xgettext &>/dev/null; then
	echo "Error: xgettext not found. Install gettext." >&2
	exit 20
fi

if ! command -v msgmerge &>/dev/null; then
	echo "Error: msgmerge not found. Install gettext." >&2
	exit 21
fi

echo "Extracting translatable strings into po/amule.pot ..."
# --no-wrap: project policy is one msgid/msgstr per line, no width-based
# wrapping. Keeps Weblate / msgmerge / hand-regen diffs to real content
# changes instead of reflow noise. Weblate's "application" component is
# configured to match.
xgettext \
	--no-wrap \
	--keyword=_ \
	--keyword=wxTRANSLATE \
	--keyword=wxPLURAL:1,2 \
	--files-from=po/POTFILES.in \
	--output=po/amule.pot \
	--from-code=UTF-8 \
	--add-comments=TRANSLATORS \
	--copyright-holder='Free Software Foundation, Inc.' \
	--package-name='aMule' \
	--package-version='GIT' \
	--msgid-bugs-address='https://github.com/amule-org/amule/issues'
die 30 "xgettext failed"

# xgettext writes "Copyright (C) YEAR" as a placeholder; fill it in.
YEAR=$(date +%Y)
sed -e "1,5 s/^# Copyright (C) YEAR /# Copyright (C) ${YEAR} /" po/amule.pot > po/amule.pot.tmp \
	&& mv po/amule.pot.tmp po/amule.pot
die 32 "failed to substitute copyright year in po/amule.pot"

echo "Merging po/amule.pot into each .po file ..."
# Write via --output-file + mv rather than --update: msgmerge --update
# treats the .po as up-to-date when only the pot's wrap differs from
# the .po, and silently skips rewriting -- which defeats --no-wrap on
# any .po that was previously committed in wrapped form.
for PO_FILE in po/*.po; do
	echo "  $PO_FILE"
	msgmerge --no-wrap "$PO_FILE" po/amule.pot --output-file="$PO_FILE.tmp"
	die 31 "msgmerge failed for $PO_FILE"
	mv "$PO_FILE.tmp" "$PO_FILE"
	die 31 "failed to install merged $PO_FILE"
done

echo "Done."
exit 0
