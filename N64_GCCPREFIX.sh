# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

if [ -z "${N64_GCCPREFIX-}" ]; then
    if [ -z "${N64_INST-}" ]; then
        echo Neither N64_GCCPREFIX nor N64_INST is set
        exit 1
    fi
    export N64_GCCPREFIX="${N64_INST}"
    echo "N64_GCCPREFIX set to ${N64_GCCPREFIX} (from N64_INST)"
else
    echo "N64_GCCPREFIX kept as ${N64_GCCPREFIX}"
fi
