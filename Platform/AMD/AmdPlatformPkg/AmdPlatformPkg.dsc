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
  #
  # GCC -fstack-protector requires __stack_chk_guard / __stack_chk_fail.
  #
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf
  AlwaysFalseDepexLib|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  IpmiLib|MdeModulePkg/Library/BaseIpmiLibNull/BaseIpmiLibNull.inf
  IpmiCommandLib|MdeModulePkg/Library/BaseIpmiCommandLibNull/BaseIpmiCommandLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PlatformPKProtectionLib|SecurityPkg/Library/PlatformPKProtectionLibVarPolicy/PlatformPKProtectionLibVarPolicy.inf
  PlatformSocLib|AmdPlatformPkg/Library/DxePlatformSocLib/DxePlatformSocLibNull.inf
  SpiHcPlatformLib|AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  SecureBootVariableProvisionLib|SecurityPkg/Library/SecureBootVariableProvisionLib/SecureBootVariableProvisionLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLibNull/PeiDxeTpmPlatformHierarchyLib.inf
  Tpm2HelpLib|SecurityPkg/Library/Tpm2HelpLib/Tpm2HelpLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf

  !if $(TARGET) == RELEASE
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !else
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !endif

  AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf
  AmdPspMboxLibV2|AgesaModulePkg/Library/AmdPspMboxLibV2/AmdPspMboxLibV2.inf
  AmdPspBaseLibV2|AgesaModulePkg/Library/AmdPspBaseLibV2/AmdPspBaseLibV2.inf
  FchBaseLib|AgesaModulePkg/Library/FchBaseLib/FchBaseLib.inf

  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  AmlGenerationLib|AgesaModulePkg/Library/DxeAmlGenerationLib/AmlGenerationLib.inf
  SmnAccessLib|AgesaModulePkg/Library/SmnAccessLib/SmnAccessLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf

  # Simulator serial port 80
  SerialPortLib|AmdPlatformPkg/Library/SimulatorSerialPortLibPort80/SimulatorSerialPortLibPort80.inf

  # Boot logo

  NbioHandleLib|AgesaModulePkg/Library/NbioHandleLib/NbioHandleLib.inf
  PcieConfigLib|AgesaModulePkg/Library/PcieConfigLib/PcieConfigLib.inf

  FchEspiCmdLib|AgesaModulePkg/Library/FchEspiCmdLib/FchEspiCmdLib.inf
  AmdPostCodeLib|AmdPlatformPkg/Library/AmdPostCodeLibNull/AmdPostCodeLibNull.inf

[LibraryClasses.common.PEIM]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiTcg2PhysicalPresenceLib|SecurityPkg/Library/PeiTcg2PhysicalPresenceLib/PeiTcg2PhysicalPresenceLib.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  SmmRelocationLib|UefiCpuPkg/Library/SmmRelocationLib/AmdSmmRelocationLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  CcExitLib|UefiCpuPkg/Library/CcExitLibNull/CcExitLibNull.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.DXE_SMM_DRIVER, LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  AmdPspRomArmorLib|AgesaModulePkg/Library/AmdPspRomArmorLibNull/AmdPspRomArmorLibNull.inf
  PlatformPspRomArmorWhitelistLib|AgesaPkg/Addendum/Psp/PspRomArmorWhitelistLib/PspRomArmorWhitelistLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf

[LibraryClasses.Common.DXE_DRIVER]
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  HstiLib|MdePkg/Library/DxeHstiLib/DxeHstiLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  PciSegmentInfoLib|MdePkg/Library/BasePciSegmentInfoLibNull/BasePciSegmentInfoLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.inf
  Tcg2PpVendorLib|SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
  AmdPspRomArmorLib|AgesaModulePkg/Library/AmdPspRomArmorLibNull/AmdPspRomArmorLibNull.inf

[LibraryClasses.Common.SMM_CORE]
  SmmCorePlatformHookLib|AmdPlatformPkg/Library/SmmCorePlatformHookLib/SmmCorePlatformHookLib.inf
  SmmCoreAmdSpiHcHookHookLib|AmdPlatformPkg/Library/SmmCoreAmdSpiHcHookLib/SmmCoreAmdSpiHcHookLib.inf

[Components.common.DXE_DRIVER]
  AmdPlatformPkg/Universal/SmbiosCommonDxe/SmbiosCommonDxe.inf                               # AMD SMBIOS common DXE driver

[Components]
  AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
  AmdPlatformPkg/Library/AmdPspTcgDonePeiDepexLib/AmdPspTcgDonePeiDepexLib.inf
  AmdPlatformPkg/Library/AmdCpmGpioInitFinishedDepexLib/AmdCpmGpioInitFinishedDepexLib.inf
  AmdPlatformPkg/Library/CcxTscTimerLib/BaseTscTimerLib.inf
  AmdPlatformPkg/Library/AmdPostCodeLibNull/AmdPostCodeLibNull.inf
  AmdPlatformPkg/Library/EmulatorSerialPort80RedirectLib/EmulatorSerialPort80Redirect.inf
  AmdPlatformPkg/Library/DxePlatformSocLib/DxePlatformSocLibNull.inf
  AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibNull.inf
  AmdPlatformPkg/Library/SimulatorSerialPortLibPort80/SimulatorSerialPortLibPort80.inf
  AmdPlatformPkg/Universal/SecureBoot/SecureBootDefaultKeysInit/SecureBootDefaultKeysInit.inf
  AmdPlatformPkg/Universal/HiiConfigRouting/AmdConfigRouting.inf
  AmdPlatformPkg/Universal/LogoDxe/JpegLogoDxe.inf                                           # Server platform JPEG logo driver
  AmdPlatformPkg/Universal/LogoDxe/LogoDxe.inf                                               # Server platfrom Bitmap logo driver
  AmdPlatformPkg/Universal/LogoDxe/S3LogoDxe.inf

  AmdPlatformPkg/Library/SmmCorePlatformHookLib/SmmCorePlatformHookLib.inf
  AmdPlatformPkg/Library/SmmCoreAmdSpiHcHookLib/SmmCoreAmdSpiHcHookLib.inf
  AgesaModulePkg/Nbio/Library/CommonDxe/NbioCommonDxeLib.inf
  AgesaModulePkg/Library/PcieConfigLib/PcieConfigLib.inf

[Components.IA32]
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf
  AmdPlatformPkg/Library/CcxTscTimerLib/PeiTscTimerLib.inf
  AmdPlatformPkg/Universal/Hob/PlatformHobPei/PlatformHobPei.inf

[Components.X64]
  AmdPlatformPkg/Library/CcxTscTimerLib/DxeTscTimerLib.inf
  AmdPlatformPkg/Library/LocalApicTimerHpetSyncLib/LocalApicTimerHpetSyncLib.inf
  AmdPlatformPkg/Library/PlatformRedfishCredentialLib/PlatformRedfishCredentialLib.inf
  AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibDxe.inf
  AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibSmm.inf
  AmdPlatformPkg/Universal/Spi/EspiNorFlash/EspiNorFlashSmm.inf
  AmdPlatformPkg/Universal/Acpi/AcpiCommon/AcpiCommon.inf
  AmdPlatformPkg/Universal/Spi/AmdSpiFvb/AmdSpiFvbSmm.inf
  AmdPlatformPkg/Universal/Spi/AmdSpiFvb/AmdSpiFvbDxe.inf
  AmdPlatformPkg/Universal/Spi/AmdSpiHc/AmdSpiHcSmm.inf
  AmdPlatformPkg/Universal/Spi/AmdSpiHc/AmdSpiHcDxe.inf
  AmdPlatformPkg/Universal/Spi/BoardSpiBus/BoardSpiBusDxe.inf
  AmdPlatformPkg/Universal/Spi/BoardSpiBus/BoardSpiBusSmm.inf
  AmdPlatformPkg/Universal/Spi/BoardSpiConfig/BoardSpiConfigDxe.inf
  AmdPlatformPkg/Universal/Spi/BoardSpiConfig/BoardSpiConfigSmm.inf

  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiDsdtLib/AcpiDsdtLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtCpuTopologyLib/AcpiSsdtCpuTopologyLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtEspiUartLib/AcpiSsdtEspiUartLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtPciLib/AcpiSsdtPciLib.inf
  AmdPlatformPkg/DynamicTables/Library/Acpi/AcpiSsdtSerialLib/AcpiSsdtSerialLib.inf
  AmdPlatformPkg/DynamicTables/Library/SampleCmPlatOverrideLib/SamplecmPlatOverrideLib.inf

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
