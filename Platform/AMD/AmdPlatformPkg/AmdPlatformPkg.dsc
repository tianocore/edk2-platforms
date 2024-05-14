## @file
# AMD Platform common Package DSC file
# This is the package provides the AMD edk2 common platform drivers
# and libraries for AMD Server, Clinet and Gaming console platforms.
#
# Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = AmdPlatformPkg
  PLATFORM_GUID                  = ACFD1C98-D451-45FE-B300-4049C5AD553B
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 1.28
  OUTPUT_DIRECTORY               = Build/AmdPlatformPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

[Packages]
  AmdPlatformPkg/AmdPlatformPkg.dec

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.Common]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  !if $(TARGET) == RELEASE
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !else
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !endif

[LibraryClasses.common.DXE_DRIVER]
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[Components]
  AmdPlatformPkg/Universal/LogoDxe/JpegLogoDxe.inf                                           # Server platform JPEG logo driver
  AmdPlatformPkg/Universal/LogoDxe/LogoDxe.inf                                               # Server platfrom Bitmap logo driver
  AmdPlatformPkg/Universal/LogoDxe/S3LogoDxe.inf
