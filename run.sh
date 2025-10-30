#!/bin/bash

set -e
trap 'echo Command failed: $BASH_COMMAND' ERR

EXE=nsh
BUILD_BASE_DIR="build"

# --- Help ---
show_help() {
    echo "Usage: ./run.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -d, --debug       Build in Debug mode"
    echo "  -b, --build-only  Only build, do not run shell or tests"
    echo "  -t, --test        Build and run tests"
    echo "  -c, --clean       Remove the build directories and exit"
    echo "  -h, --help        Display this help message"
    echo ""
    echo "Default: Release build and run shell"
}

# --- Default config ---
BUILD_CONFIG="Release"
BUILD_SUB_DIR="release"
CMAKE_FLAGS=""
CLEAN=false
BUILD_ONLY=false
RUN_TESTS=false

# --- Parse arguments ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--debug)
            BUILD_CONFIG="Debug"
            BUILD_SUB_DIR="debug"
            CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON -DLOG_LEVEL=INFO"
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# --- Clean ---
if [[ "$CLEAN" = true ]]; then
    echo "Cleaning build directories..."
    rm -rf "$BUILD_BASE_DIR"
    echo -e "\e[1;32mClean done.\e[0m"
    exit 0
fi

BUILD_PATH="$BUILD_BASE_DIR/$BUILD_SUB_DIR"

# --- Configure & build ---
cmake -S . -B "$BUILD_PATH" $CMAKE_FLAGS
cmake --build "$BUILD_PATH" -j$(nproc) --target $EXE

# --- Run ---
if [[ "$BUILD_ONLY" = true ]]; then
    echo "Build completed in $BUILD_PATH"
    exit 0
fi

if [[ "$RUN_TESTS" = true ]]; then
    TEST_EXE="$BUILD_PATH/tests_novash"
    if [[ ! -f "$TEST_EXE" ]]; then
        echo "Test executable not found, building..."
        cmake --build "$BUILD_PATH" --target tests_novash
    fi
    echo "Running tests..."
    echo $TEST_EXE
    "$TEST_EXE"
    exit 0
fi

# Run the shell
"$BUILD_PATH/$EXE"
