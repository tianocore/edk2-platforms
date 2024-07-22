## @file
# AMD Platform common Package DSC file
# This is the package provides the AMD edk2 common platform drivers
# and libraries for AMD Server, Client and Gaming console platforms.
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
  AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf
  AlwaysFalseDepexLib|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
  AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PlatformPKProtectionLib|SecurityPkg/Library/PlatformPKProtectionLibVarPolicy/PlatformPKProtectionLibVarPolicy.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  SecureBootVariableProvisionLib|SecurityPkg/Library/SecureBootVariableProvisionLib/SecureBootVariableProvisionLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
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
  PlatformSocLib|AmdPlatformPkg/Library/DxePlatformSocLib/DxePlatformSocLibNull.inf
  SpiHcPlatformLib|AmdPlatformPkg/Universal/Spi/SpiFvb/SpiFvbDxe.inf

[LibraryClasses.common.SMM_CORE]
  SmmCoreAmdSpiHcHookLib|AmdPlatformPkg/Library/SmmCoreAmdSpiHcHookLib/SmmCoreAmdSpiHcHookLib.inf
  SmmCorePlatformHookLib|AmdPlatformPkg/Library/SmmCorePlatformHookLib/SmmCorePlatformHookLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  SpiHcPlatformLib|AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibSmm.inf

[Components]
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiFacsLib/AcpiFacsLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiFadtLib/AcpiFadtLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiHpetLib/AcpiHpetLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiMadtLib/AcpiMadtLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiMcfgLib/AcpiMcfgLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSpmiLib/AcpiSpmiLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtCpuTopologyLib/AcpiSsdtCpuTopologyLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtPciLib/AcpiSsdtPciLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiWsmtLib/AcpiWsmtLib.inf
  AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
  AmdPlatformPkg/Library/DxePlatformSocLib/DxePlatformSocLibNull.inf
  AmdPlatformPkg/Library/SimulatorSerialPortLibPort80/SimulatorSerialPortLibPort80.inf
  AmdPlatformPkg/Library/SmmCoreAmdSpiHcHookLib/SmmCoreAmdSpiHcHookLib.inf
  AmdPlatformPkg/Library/SmmCorePlatformHookLib/SmmCorePlatformHookLib.inf
  AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibDxe.inf
  AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibSmm.inf
  AmdPlatformPkg/Universal/HiiConfigRouting/AmdConfigRouting.inf
  AmdPlatformPkg/Universal/LogoDxe/JpegLogoDxe.inf                                           # Server platform JPEG logo driver
  AmdPlatformPkg/Universal/LogoDxe/LogoDxe.inf                                               # Server platfrom Bitmap logo driver
  AmdPlatformPkg/Universal/LogoDxe/S3LogoDxe.inf
  AmdPlatformPkg/Universal/SecureBoot/SecureBootDefaultKeysInit/SecureBootDefaultKeysInit.inf
  AmdPlatformPkg/Universal/SmbiosCommonDxe/SmbiosCommonDxe.inf
  AmdPlatformPkg/Universal/Spi/BoardSpiConfig/BoardSpiConfigDxe.inf
  AmdPlatformPkg/Universal/Spi/BoardSpiConfig/BoardSpiConfigSmm.inf
  AmdPlatformPkg/Universal/Spi/SpiFvb/SpiFvbDxe.inf
  AmdPlatformPkg/Universal/Spi/SpiFvb/SpiFvbSmm.inf
