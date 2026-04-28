#! /bin/bash

die () {
	local ERR_CODE=$?
	[[ $ERR_CODE == 0 ]] && return

	local EXIT_CODE=$1
	shift
	local MESSAGE=( "$@" )

	echo "${MESSAGE[@]}"
	exit "${EXIT_CODE}"
}

cmake_configure () {
	rm -rf build
	die 21 "Error trying to remove the build folder"

	cmake \
		-B build \
		-DCMAKE_BUILD_TYPE=Debug \
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
	cmake --build build "$@"

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
die 1 \
	" The current path is: '${PWD}'" \
	$'\n' \
	"This script must be run in '${GIT_ROOT}'"

cmake_configure
cmake_build "$@"
cmake_test

exit 0
