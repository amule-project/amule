#!/bin/bash
# aMule wrapper script with cleaner output
# Prevents wxWidgets 3.2+ sizer warnings and filters GTK warnings

# Set environment variables to suppress warnings
export WXSUPPRESS_SIZER_FLAGS_CHECK=1

echo "Starting aMule with cleaner output..."
echo "WXSUPPRESS_SIZER_FLAGS_CHECK=1"

# Change to the correct directory and run aMule, filtering out common warnings
cd /work/git/amule/build/src && ./amule "$@" 2> >(
    grep -v -E "Gtk-CRITICAL|pixman|Failed to load module|colorreload|window-decorations"
)
