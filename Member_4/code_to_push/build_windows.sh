#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
if [ "$MSYSTEM" != "UCRT64" ]; then
    echo "Open MSYS2 UCRT64, then run: bash build_windows.sh"
    exit 1
fi
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
if [ "${1:-}" != "--no-run" ]; then
    ./build/BarcodeStudio.exe
fi
