#!/usr/bin/env bash
set -euo pipefail

build_type="Debug"

case "${1:-debug}" in
  debug)
    build_type="Debug"
    ;;
  release)
    build_type="Release"
    ;;
  clean)
    rm -rf build
    echo "Removed build/"
    exit 0
    ;;
  rebuild)
    rm -rf build
    build_type="Debug"
    ;;
  *)
    echo "Usage: ./m [debug|release|clean|rebuild]" >&2
    exit 2
    ;;
esac

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE="${build_type}"
cmake --build build
