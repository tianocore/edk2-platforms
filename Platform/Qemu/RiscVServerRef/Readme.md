# Overview

This directory holds the UEFI implementation for the RiscVServerRef machine,
which is a fully software emulated RISC-V server platform. It implements the
RISC-V Server Platform Specification v1.0.0 [1], and aims to emulate real
hardware as closely as possible. Its purpose is to enable new feature
development when hardware for RISC-V server platform is not yet available. It
allows firmware-to-hardware interaction simulations and debugging that would be
difficult or impossible on real hardware.

Keep in mind that all of the above is possible because this machine is fully
emulated in software. RiscVServerRef does not use any hardware acceleration
of your platform, even if you run it on a RISC-V platform.

[1] https://github.com/riscv/riscv-server-platform-specification

# How to build (Linux Environment)

## Prerequisites

The build process for RiscVServerRef uses an FDF file for flash image
composition. Two flash images are produced:
- `RISCV_SERVER_REF_CODE.fd` — firmware code (SEC + PEI + DXE)
- `RISCV_SERVER_REF_VARS.fd` — UEFI variable store

## Obtaining source code

Create a directory `$WORKSPACE` to hold the source code of the components.

  1. [qemu](https://gitlab.com/qemu-project/qemu.git)
  2. [edk2](https://github.com/tianocore/edk2)
  3. [edk2-platforms](https://github.com/tianocore/edk2-platforms)
  4. [edk2-non-osi](https://github.com/tianocore/edk2-non-osi)

## Manual building

1. Compile QEMU

  Support for the `riscv-server-ref` machine used as the basis for this platform
  is available in QEMU vx.x.x and later. If your distribution provides an
  earlier version, compile QEMU from source.

  Set `$INSTALL_PATH` to `/usr/local`, `~/local`, or any preferred location.

  ```
  cd $WORKSPACE/qemu
  mkdir -p build-native
  cd build-native
  ../configure --target-list=riscv64-softmmu --prefix=$INSTALL_PATH
  make install
  ```

  QEMU should now be installed in `$INSTALL_PATH`.

2. Compile UEFI for the RiscVServerRef platform

  Detailed build instructions can be found at:
  https://github.com/tianocore/edk2-platforms

  The following is a short description to build for the RiscVServerRef platform.

  Compile BaseTools and prepare the environment:

  ```
  cd $WORKSPACE
  export PACKAGES_PATH=$WORKSPACE/edk2:$WORKSPACE/edk2-platforms:$WORKSPACE/edk2-non-osi
  make -C edk2/BaseTools
  . edk2/edksetup.sh
  ```

  Now compile UEFI for RiscVServerRef:

  ```
  cd $WORKSPACE
  build -b RELEASE -a RISCV64 -t GCC -p edk2-platforms/Platform/Qemu/RiscVServerRef/RiscVServerRef.dsc
  ```

  Copy the flash images to the top `$WORKSPACE` directory:

  ```
  cp Build/RiscVServerRef/RELEASE_GCC/FV/RISCV_SERVER_REF_CODE.fd .
  cp Build/RiscVServerRef/RELEASE_GCC/FV/RISCV_SERVER_REF_VARS.fd .
  ```

# Running

  The resulting `RISCV_SERVER_REF_CODE.fd` file contains the firmware code
  image and `RISCV_SERVER_REF_VARS.fd` contains the UEFI variable store.

  Boot to the UEFI console with the following QEMU command:

  ```
  $INSTALL_PATH/qemu-system-riscv64 \
    -nographic \
    -machine riscv-server-ref \
    -smp 2 \
    -m 4G \
    -drive file=RISCV_SERVER_REF_CODE.fd,if=pflash,format=raw,unit=0 \
    -drive file=RISCV_SERVER_REF_VARS.fd,if=pflash,format=raw,unit=1 \
    -drive file=ubuntu-xx.xx-riscv64.img,format=raw,if=ide,id=hd0 \
    -netdev user,id=net0 -device e1000,netdev=net0
  ```
