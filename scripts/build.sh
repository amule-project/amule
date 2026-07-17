#!/usr/bin/env bash

# set -euo pipefail

readonly ROOT_DIR=$(dirname $(dirname "$(realpath -e "${BASH_SOURCE[0]}")"))

OPTIONS=
OPTIONS="${OPTIONS} --prefix=/home/aubin/bin/amule"
OPTIONS="${OPTIONS} --with-denoise-level=3"
OPTIONS="${OPTIONS} --enable-optimize"
OPTIONS="${OPTIONS} --enable-upnp"
OPTIONS="${OPTIONS} --enable-geoip"
OPTIONS="${OPTIONS} --enable-nls"
OPTIONS="${OPTIONS} --enable-amule-daemon"
OPTIONS="${OPTIONS} --enable-amulecmd"
OPTIONS="${OPTIONS} --enable-mmap"
OPTIONS="${OPTIONS} --enable-ccache"
OPTIONS="${OPTIONS} --enable-fileview"

OPTIONS="${OPTIONS} --disable-debug"
# OPTIONS="${OPTIONS} --enable-debug"
OPTIONS="${OPTIONS} --disable-monolithic"

mkdir -p "${ROOT_DIR}/build"
if cd "${ROOT_DIR}/build"; then
   "${ROOT_DIR}/configure" ${OPTIONS}
   make clean
   bear --output ${ROOT_DIR}/.vscode/compile_commands.json -- make -j$(nproc)
fi
