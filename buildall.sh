#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

set -ex

njobs=$(nproc)

pushd f3dex2
./build_libdragon.sh
make -j${njobs}
popd
