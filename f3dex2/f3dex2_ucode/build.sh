#!/usr/bin/env sh
# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

set -ex

if command -v armips >/dev/null 2>&1; then
    export ARMIPS=armips
else
    mkdir -p build/armips
    cd build/armips
    cmake -DCMAKE_BUILD_TYPE=Release ../../armips
    cmake --build . -j$(nproc)
    export ARMIPS="$(realpath armips)"
    cd ../..
fi

make -C f3dex2 F3DZEX_NoN_2.08J
