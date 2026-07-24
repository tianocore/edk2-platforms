# NordMinPlatformPkg

This project brings UEFI support to Nord (AARCH64) following the MinPlatform specification.

## Capabilities
- Boot UEFI Shell, and UiApp

## How to build
- Use the provided build wrapper from the workspace root:
  ```bash
  cd edk2-platforms/Platform/Qualcomm/NordFamily/NordMinPlatformPkg
  ./BuildNordMinPlatformPkg.sh
  ```

- To build directly, use the upstream edk2-platforms package path:
  ```bash
  GCC_AARCH64_PREFIX=aarch64-linux-gnu- build -b DEBUG -a AARCH64 -t GCC -p edk2-platforms/Platform/Qualcomm/NordFamily/NordMinPlatformPkg/NordMinPlatformPkg.dsc -j $WORKSPACE/Build/NordMinPlatformPkg.log
  ```

### Prerequisites
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
    - edk2-platforms/Platform/Qualcomm/NordFamily
    - edk2-platforms/Platform/Qualcomm/Common
    - edk2-platforms/Silicon/Qualcomm
    - edk2-platforms/Silicon/Qualcomm/Common
    - edk2-qualcomm-extra


## How to use

## Important notes
- Secure boot is not available .
