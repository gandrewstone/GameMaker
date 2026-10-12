#!/bin/sh
# Builds the port on Linux and macOS (build.bat does the same on Windows).
#
# Needs: cmake, a C and C++ compiler (gcc or clang), git (CMake fetches raylib and ymfm), and on Linux the
# development packages raylib's window code needs, e.g. on Debian/Ubuntu:
#   sudo apt install build-essential cmake git libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
#
# usage: ./build.sh [extra cmake --build arguments]       GM_BUILD_DIR=<dir> changes the build folder (default ./build),
#                                                        GM_BUILD_TYPE=RelWithDebInfo (or Debug) for a build gdb can read
set -e
here=$(cd "$(dirname "$0")" && pwd)
dir=${GM_BUILD_DIR:-$here/build}
gen=""
if command -v ninja >/dev/null 2>&1; then gen="-G Ninja"; fi
if [ ! -f "$dir/CMakeCache.txt" ]; then
  cmake -S "$here" -B "$dir" $gen -DCMAKE_BUILD_TYPE=${GM_BUILD_TYPE:-Release}
fi
cmake --build "$dir" "$@"
