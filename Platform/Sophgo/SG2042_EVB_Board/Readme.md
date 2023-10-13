# Introduction to Sophgo SG2042 Platform #


This document provides guidelines for building UEFI firmware for Sophgo SG2042.
Sophgo SG2042 is a 64 and processor of RISC-V architecture.
Sophgo SG2042 UEFI can currently use Opensbi+UEFI firmware+GRUB to successfully enter the Linux distribution.

## How to build (X86 Linux Environment)

### SG2042 EDK2 Initial Environment  ###

**statement**：The operating environment of this project is deployed on the Sophgo original environment.

1. Build Sophgo SG2042 original environment，the specific compilation process is https://github.com/sophgo/sophgo-doc/tree/main/SG2042/HowTo.
    Note: The ZSBL mentioned in the original environment is later called FSBL.

2. Install package on ubuntu

     ```
     sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build uuide-dev
     ```

3. Follow edk2-platforms/Readme.md to obtaining source code, and config build env. For Example:

   ```
   export WORKSPACE=/work/git/tianocore
   mkdir -p $WORKSPACE
   cd $WORKSPACE
   git clone https://github.com/tianocore/edk2.git
   cd edk2
   git submodule update --init
   cd ..
   git clone https://github.com/tianocore/edk2-platforms.git
   cd edk2-platforms
   git submodule update --init
   cd ..
   git clone https://github.com/tianocore/edk2-non-osi.git
   export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi
   ```

4. Build

   4.1 Using GCC toolchain

   ```
   export GCC5_RISCV64_PREFIX=riscv64-linux-gnu-
   export PYTHON_COMMAND=python3
   export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
   source edk2/edksetup.sh --reconfig
   make -C edk2/BaseTools
   source edk2/edksetup.sh BaseTools
   build -a RISCV64 -t GCC5 -p Platform/Sophgo/SG2042_EVB_Board/SG2042.dsc
   ```

   4.2 Using CLANGDWARF toolchain (clang + lld)

   **statement**：Our team tried to compile the port using the CLANGDWARF toolchain (clang version 18.0.0). It was able to build successfully but the compiled binary was not fully work.

   ```
   export CLANGDWARF_BIN=${CLANGDWARF_PATH}/build/bin/
   export PYTHON_COMMAND=python3
   export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
   source edk2/edksetup.sh --reconfig
   make -C edk2/BaseTools
   source edk2/edksetup.sh BaseTools
   build -a RISCV64 -t CLANGDWARF -p Platform/Sophgo/SG2042_EVB_Board/SG2042.dsc
   ```

   After a successful build, the resulting images can be found in Build/{Platform Name}/{TARGET}_{TOOL_CHAIN_TAG}/FV/SG2042.fd.

5. The SG2042.fd file will be renamed to riscv64_Image using the "mv" command.

   ```
   mv SG2042.fd riscv64_Image
   ```

6. Now go to replace the original riscv64_Image file under SD boot, then you can enter the EDK2 Shell.

7. Use GRUB2 to boot linux OS

   Refer to https://github.com/sophgo/sophgo-doc/tree/main/SG2042/HowTo (How to build and config grub2.rst) build of GRUB2, or use the built (https://github.com/AII-SDU/GRUB.git). Put the built files into the fs0: directory for execution.

   Note: Currently, if you want to boot Linux OS via GRUB2, you can only plug in one DDR, otherwise GRUB2 will hit relocation overflow error. There is currently a problem with relocation overflow on RISC-V with multi-range memory layout, and workaround is work in progress.


## Platform Status ##
**SG2042_EVB_Board** Currently the binary built from SG2042 edk2 package can boot Sophgo SG2042 EVB to EFI shell with console, boot the operating system using GRUB2 into the Linux operating system for execution. Please refer to
https://github.com/AII-SDU/edk2-platforms/blob/devel-Sophgo/SG2042Pkg/Platform/Sophgo/About_Sophgo_platform.md for the boot process.

## Supported Operating Systems
The preliminary running test of the following operating systems has been completed on the EVB test board, and the desktop environment has been deployed.
1. Ubuntu
2. Fedora
3. openKylin
4. opemEuler

## Known Issues and Limitations
This test only runs on SG2042 EVB with RISC-V RV64 architecture





