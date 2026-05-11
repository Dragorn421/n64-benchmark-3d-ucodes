#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

set -ex

njobs=$(nproc)

pushd f3dex2
./build_libdragon.sh
pushd f3dex2_ucode
./build.sh
popd
pushd assets
make
popd
make -j${njobs}
popd
