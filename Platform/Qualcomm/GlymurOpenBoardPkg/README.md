# GlymurOpenBoardPkg

This project brings UEFI support to Glymur (AARCH64) following the MinPlatform specification.

## Capabilities
- Boot UEFI Shell, and UiApp

## How to build
- GCC_AARCH64_PREFIX=aarch64-linux-gnu- build -b DEBUG -a AARCH64  -D PEI_ARCH=AARCH64 -D DXE_ARCH=AARCH64 -t GCC -p edk2-qualcomm/GlymurOpenBoardPkg/GlymurOpenBoardPkg.dsc -j $WORKSPACE/Build/GlymurOpenBoardPkg.log

### Prerequesites
- Ensure cross compiler is setup: aarch64-linux-gnu-gcc

- EDK2
  - How to setup a local tree: https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II

- EDK2 Platforms
  - https://github.com/tianocore/edk2-platforms

- Environment variables:
  - WORKSPACE set to your current workspace
  - PACKAGES_PATH should contain path to:
    - edk2
    - edk2-platforms
    - edk2-platforms/Platform
    - edk2/ArmPlatformPkg
    - edk2-platforms/Platform/Qualcomm
    - edk2-platforms/Silicon/Qualcomm

## Important notes
- Secure boot is not available.
