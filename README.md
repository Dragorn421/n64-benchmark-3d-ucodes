# n64-benchmark-3d-ucodes

This repository is about building test roms for benchmarking the performance of different microcodes for 3D graphics on the Nintendo 64.

This is a work in progress!

## Cloning

This repo uses git submodules. Clone with

```
git clone --recurse-submodules git@github.com:Dragorn421/n64-benchmark-3d-ucodes.git
```

or

```
git clone git@github.com:Dragorn421/n64-benchmark-3d-ucodes.git
git submodule update --init --recursive
```

## Building

To build all roms, run the `buildall.sh` script.

The only requirement is that you have the libdragon toolchain (gcc and binutils) installed at `$N64_INST` or `$N64_GCCPREFIX`.
