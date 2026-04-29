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
	echo "Compiles the program"
	echo
	echo "Usage: $0 [-d] [-h | -?]"
	echo "  -d    Enable debug compilation (default is release)"
	echo "  -h    Display this help message"
	echo "  -?    Display this help message"
}

OPT_DEBUG=Release
OPT_J=1

# Setup parse options
# -o "j:" means short flag 'j' requires an argument
# --long "jobs:" means long flag 'jobs' requires an argument
if ! PARAMS=$(getopt -o "dhj:" -l "debug,jobs:,help" -n "$0" -- "$@"); then
	# If getopt fails (invalid flag), exit
	usage
	false; die 10
fi

# Re-set the positional parameters to the cleaned version from getopt
eval set -- "$PARAMS"

while true; do
	case "$1" in
	-d | --debug )
		OPT_DEBUG=Debug
		echo "[DEBUG compilation ENABLED]"
		shift
		;;
	-h | --help )
		usage
		false; die 0
		;;
	-j | --jobs )
		if [[ -n "$2" && "$2" != -* ]]; then
			OPT_J="$2"
			shift 2 # Move past the flag AND the value
		else
			echo "Error: -j requires a non-empty argument." >&2
			usage
			false; die 12
		fi
		;;
	-- )
		shift
		break;
		;;
	* )
		usage
		false; die 12 "Error processing command line parameters"
		;;
	esac
done

cmake_configure () {
	rm -rf build
	die 21 "Error trying to remove the build folder"

	cmake \
		-B build \
		-DCMAKE_BUILD_TYPE=${OPT_DEBUG} \
		-DBUILD_ALC=YES \
		-DBUILD_ALCC=YES \
		-DBUILD_AMULECMD=YES \
		-DBUILD_CAS=YES \
		-DBUILD_DAEMON=YES \
		-DBUILD_WXCAS=YES \
		-DBUILD_ED2K=YES \
		-DBUILD_MONOLITHIC=YES \
		-DBUILD_REMOTEGUI=YES \
		-DBUILD_TESTING=YES \
		-DBUILD_WEBSERVER=YES \
		-DENABLE_NLS=YES \
		-DENABLE_UPNP=YES \
		-DENABLE_IP2COUNTRY=YES

	die 22 "CMake configuration failed"
}

cmake_build() {
	cmake --build build -j"${OPT_J}" "$@"

	die 3 "CMake build failed"
}

cmake_test() {
	ctest \
		--test-dir build \
		--output-on-failure \
		--timeout 10

	die 4 "CMake test failed"
}

GIT_ROOT=$(git rev-parse --show-toplevel)
[[ ${PWD} == "${GIT_ROOT}" ]]
die 12 \
	" The current path is: '${PWD}'" \
	$'\n' \
	"This script must be run in '${GIT_ROOT}'"

cmake_configure
cmake_build "$@"
cmake_test

exit 0
