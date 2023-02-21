# Overview
Build all advanced features for testing build and integration.

Please refer to individual feature packages for details on intended uses.

# High-Level Theory of Operation
Enable all features and build a reasonable default configuration.  This is not expected to produce binaries that are directly usable in a product as features may require board or silicon specific configuration and libraries.

## Firmware Volumes
Produces:
* FvAdvancedPreMemory
* FvAdvancedUncompressed
* FvAdvanced

## Build
Supported build targets
* VS2019
* CLANGPDB
* GCC5

## Windows Example:
git clone https://github.com/tianocore/edk2.git

git clone https://github.com/tianocore/edk2-non-osi.git

git clone https://github.com/tianocore/edk2-platforms.git

set workspace=%cd%

set EDK_TOOLS_PATH=%workspace%\edk2\BaseTools

set packages_path=%workspace%\edk2;%workspace%\edk2-non-osi;%workspace%\edk2-platforms\Platform\Intel;%workspace%\edk2-platforms\Silicon\Intel;%workspace%\edk2-platforms\Platform\Qemu;%workspace%\edk2-platforms\Silicon\Qemu;%workspace%\edk2-platforms\Features\Intel;%workspace%\edk2-platforms\Features\Intel\Debugging;%workspace%\edk2-platforms\Features\Intel\Network;%workspace%\edk2-platforms\Features\Intel\OutOfBandManagement;%workspace%\edk2-platforms\Features\Intel\PowerManagement;%workspace%\edk2-platforms\Features\Intel\SystemInformation;%workspace%\edk2-platforms\Features\Intel\UserInterface

cd %workspace%\edk2

git submodule update --init

edksetup.bat Rebuild

build -a IA32 -a X64 -t CLANGPDB -b NOOPT -p AdvancedFeaturePkg\AdvancedFeaturePkg.dsc
