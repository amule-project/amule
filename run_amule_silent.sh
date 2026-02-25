#!/bin/bash
# aMule launcher - suppresses GTK module warnings

# Suppress GTK module warnings
export GTK_MODULE_DIR=""

# Suppress wxWidgets sizer assertions
export WXSUPPRESS_SIZER_FLAGS_CHECK=1

# Run aMule
cd "$(dirname "$0")"
./src/amule "$@"