#!/bin/bash

set -e  # stop immediately if any command fails
trap 'echo "Command failed: $BASH_COMMAND"' ERR

# check cmake
if ! command -v cmake > /dev/null 2>&1; then
    echo "cmake should be installed on your system to build nsh"
    exit 1
fi

# default CMake options
CMAKE_FLAGS="-S . -B build"
BUILD_CONFIG="Release"

# enable debug and sanitizers if --debug is passed
if [[ "$1" == "--debug" ]]; then
    CMAKE_FLAGS="$CMAKE_FLAGS -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON"
    BUILD_CONFIG="Debug"
elif [[ "$1" == "--clean" ]]; then
    rm -rf build
    echo -e "\e[1;32m Project has been cleaned \e[0m"
    exit 0
fi

cmake $CMAKE_FLAGS
cmake --build build --config $BUILD_CONFIG -j${nproc}
./build/nsh
