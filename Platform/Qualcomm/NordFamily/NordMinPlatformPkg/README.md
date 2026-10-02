# NordMinPlatformPkg

This project brings UEFI support to Nord (AARCH64) following the MinPlatform specification.

## Capabilities
- Boot UEFI Shell, and UiApp

## How to build
- GCC_AARCH64_PREFIX=aarch64-linux-gnu- build -b DEBUG -a AARCH64  -D PEI_ARCH=AARCH64 -D DXE_ARCH=AARCH64 -t GCC -p edk2-qualcomm/NordMinPlatformPkg/NordMinPlatformPkg.dsc -j $WORKSPACE/Build/NordMinPlatformPkg.log

### Pre-requesites
- Ensure cross compiler is setup: aarch64-linux-gnu-gcc

- EDK2
  - How to setup a local tree: https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II

- EDK2 Platforms
  - https://github.com/tianocore/edk2-platforms

- Environnements variables:
  - WORKSPACE set to your current workspace
  - PACKAGES_PATH should contain path to:
    - edk2
    - edk2-platforms
    - edk2/ArmPlatformPkg
    - edk2-platforms/Silicon/Qualcomm
    - edk2-qualcomm
    - edk2-qualcomm/QualcommMinplatformPkg/Library


## How to use

## Important notes
- Secure boot is not available .
