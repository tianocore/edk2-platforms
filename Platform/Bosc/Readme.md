# Introduction to BOSC Xiangshan Series Platform #

This document provides guidelines for building UEFI firmware for BOSC NanhuDev.
BOSC NanhuDev is a 64 and processor of RISC-V architecture.
BOSC NanhuDev UEFI can currently use Opensbi+UEFI firmware+GRUB to successfully enter the Linux.

## How to build (X86 Linux Environment)

### NanhuDev EDK2 Initial Environment  ###

**statement**：The operating environment of this project is deployed on the BOSC original environment.

1. Install package on ubuntu

     ```
     sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build uuide-dev
     ```

2. Follow edk2-platforms/Readme.md to obtaining source code, and config build env. For Example:

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

3. Build

   3.1 Using GCC toolchain

   ```
   export GCC5_RISCV64_PREFIX=riscv64-linux-gnu-
   export PYTHON_COMMAND=python3
   export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
   source edk2/edksetup.sh --reconfig
   make -C edk2/BaseTools
   source edk2/edksetup.sh BaseTools
   build --buildtarget=DEBUG -a RISCV64 -t GCC5 -p Platform/Bosc/XiangshanSeriesPkg/NanhuDev/NanhuDev.dsc
   ```

   After a successful build, the resulting images can be found in Build/{Platform Name}/{TARGET}_{TOOL_CHAIN_TAG}/FV/NANHUDEV.fd

4. When compiling Opensbi, specify that payload is NANHUDEV.fd and specify dtb path.
   make -C ~/opensbi PLATFORM=generic CROSS_COMPILE=riscv64-unknown-linux-gnu- -j FW_PAYLOAD_PATH=$(PAYLOAD) FW_FDT_PATH=$(DTB_PATH)

5. Use GRUB2 to boot linux OS

   Reference: https://fedoraproject.org/wiki/Architectures/RISC-V/GRUB2
   Copy grubriscv64.efi and Image(linux) to the root directory of the NVME partition.
## Known Issues and Limitations
This test only runs on BOSC NanhuDev with RISC-V RV64 architecture
