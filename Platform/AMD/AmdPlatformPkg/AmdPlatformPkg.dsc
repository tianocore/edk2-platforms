## @file
# AMD Platform common Package DSC file
# This is the package provides the AMD edk2 common platform drivers
# and libraries for AMD Server, Client and Gaming console platforms.
#
# Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
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

# Include AGESA module for edk2-platforms
!include AgesaModulePkg/AgesaEdk2PlatformPkg.inc.dsc
!include AmdPlatformPkg/AmdPlatformPkg.dsc.inc

[LibraryClasses.Common]
  # AmdPlatformPkg null libraries
  AmdPostCodeLib|AmdPlatformPkg/Library/AmdPostCodeLibNull/AmdPostCodeLibNull.inf
  PlatformSocLib|AmdPlatformPkg/Library/DxePlatformSocLib/DxePlatformSocLibNull.inf

  # edk2
  AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf
  AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  IpmiCommandLib|MdeModulePkg/Library/BaseIpmiCommandLibNull/BaseIpmiCommandLibNull.inf
  IpmiLib|MdeModulePkg/Library/BaseIpmiLibNull/BaseIpmiLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  SpiHcPlatformLib|MdeModulePkg/Library/BaseSpiHcPlatformLibNull/BaseSpiHcPlatformLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tpm2HelpLib|SecurityPkg/Library/Tpm2HelpLib/Tpm2HelpLib.inf
  TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLibNull/PeiDxeTpmPlatformHierarchyLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf

  # AGESA
  FchBaseLib|AgesaModulePkg/Library/FchBaseLib/FchBaseLib.inf
  FchEspiCmdLib|AgesaModulePkg/Library/FchEspiCmdLib/FchEspiCmdLib.inf
  NbioHandleLib|AgesaModulePkg/Library/NbioHandleLib/NbioHandleLib.inf
  PcieConfigLib|AgesaModulePkg/Library/PcieConfigLib/PcieConfigLib.inf
  SmnAccessLib|AgesaModulePkg/Library/SmnAccessLib/SmnAccessLib.inf

[LibraryClasses.common.PEIM]
  # edk2
  CcExitLib|UefiCpuPkg/Library/CcExitLibNull/CcExitLibNull.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  SmmRelocationLib|UefiCpuPkg/Library/SmmRelocationLib/AmdSmmRelocationLib.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.DXE_SMM_DRIVER, LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  # edk2
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  # edk2
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf

  # AGESA
  AmdEmulationAutoDetectLib|AgesaModulePkg/Library/AmdEmulationAutoDetectLibNull/AmdEmulationAutoDetectLibNull.inf
  AmdRdeDirectoryLib|AgesaModulePkg/Library/AmdRdeDirectoryLib/AmdRdeDirectoryLib.inf
  AmdRdeMediaLib|AgesaModulePkg/Library/AmdRdeMediaLibNull/AmdRdeMediaLibNull.inf
  AmdPspRomArmorLib|AgesaModulePkg/Library/AmdPspRomArmorLibNull/AmdPspRomArmorLibNull.inf
  PlatformPspRomArmorWhitelistLib|AgesaPkg/Addendum/Psp/PspRomArmorWhitelistLib/PspRomArmorWhitelistLib.inf

[LibraryClasses.Common.DXE_DRIVER]
  # edk2
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  HstiLib|MdePkg/Library/DxeHstiLib/DxeHstiLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  PciSegmentInfoLib|MdePkg/Library/BasePciSegmentInfoLibNull/BasePciSegmentInfoLibNull.inf

  # AGESA
  AmdEmulationAutoDetectLib|AgesaModulePkg/Library/AmdEmulationAutoDetectLibNull/AmdEmulationAutoDetectLibNull.inf
  AmdPspRomArmorLib|AgesaModulePkg/Library/AmdPspRomArmorLibNull/AmdPspRomArmorLibNull.inf
  AmdRdeDirectoryLib|AgesaModulePkg/Library/AmdRdeDirectoryLib/AmdRdeDirectoryLib.inf
  AmdRdeMediaLib|AgesaModulePkg/Library/AmdRdeMediaLibNull/AmdRdeMediaLibNull.inf
  NbioCommonDxeLib|AgesaModulePkg/Nbio/Library/CommonDxe/NbioCommonDxeLib.inf

[Components]
  AmdPlatformPkg/Library/AmdCpmGpioInitFinishedDepexLib/AmdCpmGpioInitFinishedDepexLib.inf
  AmdPlatformPkg/Library/AmdPostCodeLibNull/AmdPostCodeLibNull.inf
  AmdPlatformPkg/Library/AmdPspTcgDonePeiDepexLib/AmdPspTcgDonePeiDepexLib.inf
  AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
  AmdPlatformPkg/Library/CcxTscTimerLib/BaseTscTimerLib.inf
  AmdPlatformPkg/Library/EmulatorSerialPort80RedirectLib/EmulatorSerialPort80Redirect.inf
  AmdPlatformPkg/Library/SimulatorSerialPortLibPort80/SimulatorSerialPortLibPort80.inf

[Components.IA32]
  AmdPlatformPkg/Library/CcxTscTimerLib/PeiTscTimerLib.inf

[Components.X64]
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiDsdtLib/AcpiDsdtLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtCpuTopologyLib/AcpiSsdtCpuTopologyLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtEspiUartLib/AcpiSsdtEspiUartLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtPciLib/AcpiSsdtPciLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtSerialLib/AcpiSsdtSerialLib.inf
  AmdPlatformPkg/DynamicTables/Library/SampleCmPlatOverrideLib/SamplecmPlatOverrideLib.inf
  AmdPlatformPkg/Library/AmdBdsBootConfigLib/AmdBdsBootConfigLib.inf
  AmdPlatformPkg/Library/CcxTscTimerLib/DxeTscTimerLib.inf
  AmdPlatformPkg/Library/DxePlatformSocLib/DxePlatformSocLibNull.inf
  AmdPlatformPkg/Library/LocalApicTimerHpetSyncLib/LocalApicTimerHpetSyncLib.inf
  AmdPlatformPkg/Library/PlatformRedfishCredentialLib/PlatformRedfishCredentialLib.inf
  AmdPlatformPkg/Library/SmmCoreAmdSpiHcHookLib/SmmCoreAmdSpiHcHookLib.inf
  AmdPlatformPkg/Library/SmmCorePlatformHookLib/SmmCorePlatformHookLib.inf
  AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibDxe.inf
  AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibSmm.inf
  AmdPlatformPkg/Library/SpiHcRomArmorPlatformLib/SpiHcPlatformLibDxe.inf
  AmdPlatformPkg/Library/SpiHcRomArmorPlatformLib/SpiHcPlatformLibSmm.inf

  # Not used in this platform
  AmdPlatformPkg/Universal/LogoDxe/JpegLogoDxe.inf
  AmdPlatformPkg/Universal/LogoDxe/S3LogoDxe.inf

[PcdsDynamicDefault.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x00000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0x00000000

[BuildOptions]
  GCC:*_*_*_CC_FLAGS     = -D DISABLE_NEW_DEPRECATED_INTERFACES
  INTEL:*_*_*_CC_FLAGS   = /D DISABLE_NEW_DEPRECATED_INTERFACES
  MSFT:*_*_*_CC_FLAGS    = /D DISABLE_NEW_DEPRECATED_INTERFACES

  GCC:*_*_*_CC_FLAGS     = -D USE_EDKII_HEADER_FILE

  # Turn off DEBUG messages for Release Builds
  GCC:RELEASE_*_*_CC_FLAGS     = -D MDEPKG_NDEBUG
  INTEL:RELEASE_*_*_CC_FLAGS   = /D MDEPKG_NDEBUG
  MSFT:RELEASE_*_*_CC_FLAGS    = /D MDEPKG_NDEBUG

  !ifdef $(INTERNAL_IDS)
    GCC:*_*_*_CC_FLAGS     = -DINTERNAL_IDS
    INTEL:*_*_*_CC_FLAGS   = /D INTERNAL_IDS
    MSFT:*_*_*_CC_FLAGS    = /D INTERNAL_IDS
    MSFT:*_*_*_VFRPP_FLAGS = /D INTERNAL_IDS
    MSFT:*_*_*_ASLCC_FLAGS = /D INTERNAL_IDS
    MSFT:*_*_*_ASLPP_FLAGS = /D INTERNAL_IDS
    MSFT:*_*_*_PP_FLAGS    = /D INTERNAL_IDS
    MSFT:*_*_*_APP_FLAGS   = /D INTERNAL_IDS
  !endif
