Introduction to Sophgo SG2044 Platform
============================================

This document provides guidelines for building UEFI firmware for Sophgo SG2042. Sophgo SG2042 is a 64 and processor of RISC-V architecture. Sophgo SG2042 UEFI can currently use Opensbi+UEFI firmware+GRUB to successfully enter the Linux distribution.

## How To Build (X86 Linux Environment)

### 1. Install Dependencies

-   on Ubuntu
  ```
    $ sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev libncurses-dev openssl libiberty-dev libssl-dev dkms libelf-dev libudev-dev libpci-dev golang-go qemu-user-static ninja-build uuid-dev gcc-riscv64-unknown-elf
  ```

### 2. Obtaining source code

-   Create a new folder (directory) on your local development machine for use as your workspace. This example uses /work/git/tianocore, modify as appropriate for your needs.
  ```
    $ export WORKSPACE=$PWD/work/git/tianocore
    $ mkdir -p $WORKSPACE
    $ cd $WORKSPACE
  ```

-   Into that folder, clone
   1. [edk2](https://github.com/tianocore/edk2)
   1. [edk2-platforms](https://github.com/tianocore/edk2-platforms)
   1. [edk2-non-osi](https://github.com/tianocore/edk2-non-osi) (if building
      platforms that need it)
   ```
   $ git clone https://github.com/tianocore/edk2.git
   $ cd edk2
   $ git submodule update --init
   ...
   $ git clone https://github.com/tianocore/edk2-platforms.git
   $ cd edk2-platforms
   $ git submodule update --init
   ...
   $ git clone https://github.com/tianocore/edk2-non-osi.git
   ```
-  Set up a **PACKAGES_PATH** to point to the locations of these three
   repositories:

   `$ export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi`

### 3. Manual building

-   Create a directory named 'Build' under your workspace to save the firmware

    `$ mkdir Build`

-   Set up the build environment (this will modify your environment variables)

    ```
    $ export EDK_TOOLS_PATH=$PWD/edk2/BaseTools
    $ export GCC5_RISCV64_PREFIX=riscv64-unknown-elf-
    $ source edk2/edksetup.sh
    ```

-   Build BaseTools

    `$ make -C edk2/BaseTools -j$(nproc)`

-   Start to Compile

    `$ build -a RISCV64 -t GCC5 -b DEBUG -p Platform/Sophgo/SG2044Pkg/SG2044.dsc`

-   If everything goes well, you will find SG2044.fd under Build/SG2044/DEBUG_GCC5/FV
