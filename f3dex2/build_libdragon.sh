#!/usr/bin/env sh
# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

set -e

. ../N64_GCCPREFIX.sh

mkdir -p build/libdragon
export N64_INST=$(realpath build/libdragon)

echo "N64_INST set to ${N64_INST}"

njobs=$(nproc)

set -x

make -C libdragon -j${njobs}
make -C libdragon tools -j${njobs}
make -C libdragon install
make -C libdragon tools-install
