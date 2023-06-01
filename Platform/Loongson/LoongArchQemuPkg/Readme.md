# Introduction

  This document provides the guideline to build UEFI firmware for Qemu of LoongArch.

  LoongArch is the general processor architecture of Loongson.

  We can get the latest LoongArch documents or LoongArch tools at https://github.com/loongson/.

# How to build (X86 Linux Environment)

  1. Install LoongArch cross-tools on X86 machines.
    Download cross-tools from https://github.com/loongson/build-tools ,Then config cross-tools env.
    For Example:

    $ wget https://github.com/loongson/build-tools/releases/download/2022.09.06/loongarch64-clfs-6.3-cross-tools-gcc-glibc.tar.xz
    $ tar -vxf loongarch64-clfs-6.3-cross-tools-gcc-glibc.tar.xz  -C /opt
    $ export PATH=/opt/cross-tools/bin:$PATH

    Note: Please obtain the latest cross-compilation tools from https://github.com/loongson/build-tools .

  2. Follow edk2-platforms/Readme.md to obtaining source code,And config build env.
    For Example:

    $ export WORKSPACE=/work/git/tianocore
    $ mkdir -p $WORKSPACE
    $ cd $WORKSPACE
    $ git clone https://github.com/tianocore/edk2.git
    $ cd edk2
    $ git submodule update --init
    $ cd ..
    $ git clone https://github.com/tianocore/edk2-platforms.git
    $ cd edk2-platforms
    $ git submodule update --init
    $ cd ..
    $ git clone https://github.com/tianocore/edk2-non-osi.git
    $ export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi

  3. Config  cross compiler prefix.
    For Example:

    $ export GCC5_LOONGARCH64_PREFIX=loongarch64-unknown-linux-gnu-

  4.Set up the build environment And  build BaseTool.
    For Example:

    $. edk2/edksetup.sh
    $make -C edk2/BaseTools

  5.Build  platform.
    For Exmaple:

    $build --buildtarget=DEBUG --tagname=GCC5 --arch=LOONGARCH64  --platform=Platform/Loongson/LoongArchQemuPkg/Loongson.dsc

  After a successful build, the resulting images can be found in `Build/{Platform Name}/{TARGET}_{TOOL_CHAIN_TAG}/FV/QEMU_EFI.fd`.

  A compile script is provided here:

    #!/bin/bash
    export WORKSPACE=/work/git/tianocore
    export PACKAGES_PATH=$WORKSPACE/edk2:$WORKSPACE/edk2-platforms:$WORKSPACE/edk2-non-osi
    export GCC5_LOONGARCH64_PREFIX=loongarch64-unknown-linux-gnu-
    . edk2/edksetup.sh
    make -C edk2/BaseTools
    build --buildtarget=DEBUG --tagname=GCC5 --arch=LOONGARCH64  --platform=Platform/Loongson/LoongArchQemuPkg/Loongson.dsc
