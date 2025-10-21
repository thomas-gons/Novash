#!/bin/bash

# Configuration and build script for Novash shell (nsh)
# This version uses build/debug and build/release as CMake cache directories.

set -e  # Stop immediately if any command fails
trap 'echo "Command failed: $BASH_COMMAND"' ERR

EXE=nsh
BUILD_BASE_DIR="build"

# --- Help Function ---
show_help() {
    echo "Usage: ./run.sh [OPTIONS]"
    echo ""
    echo "Builds and runs the Novash shell ($EXE)."
    echo ""
    echo "Options:"
    echo "  --debug, -d   Builds the project in Debug mode, enabling -g -O0 and sanitizers (ASAN/UBSAN)."
    echo "  --clean, -c   Removes the entire '$BUILD_BASE_DIR' directory and exits."
    echo "  --help, -h    Display this help message and exits."
    echo ""
    echo "Default mode: Release (builds in $BUILD_BASE_DIR/release)."
}

# Check if CMake is installed
if ! command -v cmake > /dev/null 2>&1; then
    echo "CMake must be installed on your system to build $EXE."
    exit 1
fi

# --- Default Configuration ---
BUILD_CONFIG="Release"
BUILD_SUB_DIR="release"
CMAKE_CONFIG_FLAGS=""
CLEAN_MODE=false # New flag to handle cleaning separately

# --- Argument Parsing ---
case "$1" in
    --debug|-d)
        BUILD_CONFIG="Debug"
        BUILD_SUB_DIR="debug"
        # Force the build type and enable sanitizers for the debug cache
        CMAKE_CONFIG_FLAGS="-DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON"
        ;;
    --clean|-c)
        CLEAN_MODE=true
        ;;
    --help|-h)
        show_help
        exit 0
        ;;
    "")
        # No argument, continue with Release defaults
        ;;
    *)
        # Unknown argument
        echo "Error: Unknown option '$1'."
        show_help
        exit 1
        ;;
esac

# --- Cleaning Execution ---
if [[ "$CLEAN_MODE" = true ]]; then
    echo "Cleaning project directories..."
    # Remove the base build directory entirely
    rm -rf "$BUILD_BASE_DIR"
    echo -e "\e[1;32mProject has been successfully cleaned.\e[0m"
    exit 0
fi

# --- Define the final, unique build path ---
# Example: build/debug or build/release
BUILD_PATH="$BUILD_BASE_DIR/$BUILD_SUB_DIR" 

# --- Phase 1: Configuration ---
cmake -S . -B "$BUILD_PATH" $CMAKE_CONFIG_FLAGS

# --- Phase 2: Build ---
cmake --build "$BUILD_PATH" -j$(nproc)

# --- Phase 3: Execution ---
# The executable path is at the root of the BUILD_PATH, e.g., build/debug/nsh
EXECUTABLE_PATH="./$BUILD_PATH/$EXE"

if [[ ! -f "$EXECUTABLE_PATH" ]]; then
    echo -e "\e[1;31mError: Executable not found at $EXECUTABLE_PATH.\e[0m"
    exit 1
fi
"$EXECUTABLE_PATH"
