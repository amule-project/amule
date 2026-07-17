#!/usr/bin/env bash

readonly ROOT_DIR=$(dirname $(dirname "$(realpath -e "${BASH_SOURCE[0]}")"))

cd ${ROOT_DIR}/build
make clean
bear --output ${ROOT_DIR}/.vscode/compile_commands.json -- make -j$(nproc)
