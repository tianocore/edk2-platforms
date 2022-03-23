# Overview
* **Feature Name:** Platform Payload
* **PI Phase(s) Supported:** SMM DXE

## Purpose
This package provides a common platform payload which is expected to work on most
Intel platforms with a bootloader.

# High-Level Theory of Operation
Bootloader initialize memory/silicon/platform and payload focus on boot media initializaiton
and OS boot. Payload is expected to be re-used on different Platforms.

In the reality, some payload modules have platform dependency (e.g. only for Intel PCH)
or bootloader dependency (only for coreboot). These modules would be located in
edk2-platform repo.

The generic UEFI payload could be built from EDK2 UefiPayloadPkg in ELF format following
universal payload specification. And a Platform Payload could be built from PlatformPayloadPkg
to provide Intel platform specific features (e.g. SPI module, PCH SMM) in FV/FD format.
This platform payload could be inserted into universal UEFI payload as an ELF section
to generate a full-feature payload.
## Firmware Volumes
* FvPlatformPayload

## Build Flows
use windows host as example to generate a full-feature payload:

** Setup the build env
set WORKSPACE=c:\payload
set PACKAGES_PATH=%WORKSPACE%\edk2;%WORKSPACE%\edk2-platforms\Features\Intel;
    %WORKSPACE%\edk2-platforms\Platform\Intel
edk2\edksetup.bat

** Build universal UEFI payload with platform Payload
python edk2-platforms\Features\Intel\PlatformPayloadPkg\PlatformPayloadPkg.py -t VS2019
  -D SMM_SUPPORT=TRUE -DVARIABLE_SUPPORT=NONE -D SMM_VARIABLE=TRUE
or
python edk2\UefiPayloadPkg\UniversalPayloadBuild.py -t VS2019 -D SMM_SUPPORT=TRUE -DVARIABLE_SUPPORT=NONE
python edk2-platforms\Features\Intel\PlatformPayloadPkg\PlatformPayloadPkg.py -t VS2019 -D SMM_VARIABLE=TRUE -s

  If build success, the final UEFI payload is at Build\UefiPayloadPkgX64\UniversalPayload.elf.

## Features

1. Modules
Currently only SMM veriable feature is available.
Several build macros are used as below for SMM variable feature modules.
!if $(SMM_VARIABLE) == TRUE
  ## PchSmiDispatchSmm
  ## FvbSmm
  ## FaultTolerantWriteSmm
  ## VariableSmm
  ## VariableSmmRuntimeDxe
!endif

2. Data Flows
SMM variable: 
The interface with bootloader are defined in PlatformPayloadPkg\Include\Guid
SpiFlashInfoGuid.h    -- SPI related information for SPI flash operation.
NvVariableInfoGuid.h  -- Variable related information for SPI NV variables.

## Control Flows
EDK2 DXE/SMM core from universal UEFI payload would dispatch all the modules
from this platform payload.

## Test Point Results
*_TODO_*
The test(s) that can verify porting is complete for the feature.

Each feature must describe at least one test point to verify the feature is successful. If the test point is not
implemented, this should be stated.

## Functional Exit Criteria
*_TODO_*
The testable functionality for the feature.

This section should provide an ordered list of criteria that a board integrator can reference to ensure the feature is
functional on their board.

## Feature Enabling Checklist
*_TODO_*
An ordered list of required activities to achieve desired functionality for the feature.

## Performance Impact
A general expectation for the impact on overall boot performance due to using this feature.

This section is expected to provide guidance on:
* How to estimate performance impact due to the feature
* How to measure performance impact of the feature
* How to manage performance impact of the feature

## Common Optimizations
*_TODO_*
Common size or performance tuning options for this feature.

This section is recommended but not required. If not used, the contents should be left empty.
