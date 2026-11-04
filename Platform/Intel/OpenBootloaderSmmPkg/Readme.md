# OpenBootloaderSmmPkg
* Overview
   This package is open-sourced in the edk2-platforms repository and provides essential SMM (System Management Mode) drivers for bootloader integration with Intel FSP (Firmware Support Package).
Package Contents
* The initial version contains code that produces FSP-I required interfaces:
   NVS_WRITE Interface - Non-Volatile Storage write operations

# Community Contributions
* The open-source community can add additional SMM drivers to this package for platform-specific requirements, such as:
  * Coreboot version of variable SMM driver
  * Custom platform SMM drivers

# Build Instructions
To build the OpenBlSmm.fd file:

BASH
```
python Platform/Intel/OpenBootloaderSmmPkg/OpenBootloaderSmmBuild.py
```

# Requirements
- EDK2 build environment
- edk2-platforms repository
- Python 3.x

* Support
This package is maintained as part of the edk2-platforms open-source project.
For issues, contributions, or questions, please use the appropriate edk2 community channels.

