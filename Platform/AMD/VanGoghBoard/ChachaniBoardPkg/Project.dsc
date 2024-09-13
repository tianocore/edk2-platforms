## @file
# EDK II Project.dsc file
#
# Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PROJECT_PKG                    = ChachaniBoardPkg

  DSC_SPECIFICATION              = 0x00010005
  PLATFORM_NAME                  = Chachani
  PLATFORM_GUID                  = 7E00A407-AF88-6B83-0187-A9075AA10AF0
  PLATFORM_VERSION               = 0.1
  FLASH_DEFINITION               = $(PROJECT_PKG)/Project.fdf
  OUTPUT_DIRECTORY               = Build/$(PROJECT_PKG)
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  #
  # Platform On/Off features are defined here
  #
  !include Board.env
  !include Set.env

  DEFINE BUILD_TYPE                = External
  DEFINE DISPATCH_BUILD            = TRUE        # if we are in Dispatch build.
  DEFINE FSPO_BUILD                = FALSE       # if we are in FSPO build.


  !if $(FSPO_BUILD) == TRUE
    #
    # Include AGESA V9 DSC file
    #
    !include AgesaModulePkg/AgesaFf3ArModulePkg.inc.dsc
    !include AmdCpmPkg/Addendum/Oem/$(BUILD_BOARD)/AmdCpmPkg.inc.dsc
  !endif

##
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
##
[SkuIds]
  0|DEFAULT     # The entry 0|DEFAULT is reserved and always required.

##
#
# Library Class section - list of all Library Classes needed by this Platform.
#
##
[LibraryClasses]
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  #
  # Generic Modules
  #
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  S3IoLib|MdePkg/Library/BaseS3IoLib/BaseS3IoLib.inf
  S3PciLib|MdePkg/Library/BaseS3PciLib/BaseS3PciLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|NetworkPkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|NetworkPkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|NetworkPkg/Library/DxeDpcLib/DxeDpcLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

  #
  # Capsule Feature
  #
  EdkiiSystemCapsuleLib|SignedCapsulePkg/Library/EdkiiSystemCapsuleLib/EdkiiSystemCapsuleLib.inf
  FmpAuthenticationLib|MdeModulePkg/Library/FmpAuthenticationLibNull/FmpAuthenticationLibNull.inf
  IniParsingLib|SignedCapsulePkg/Library/IniParsingLib/IniParsingLib.inf
  PlatformFlashAccessLib|VanGoghCommonPkg/Library/PlatformFlashAccessLib/PlatformFlashAccessLib.inf
  CapsuleHookLib|ChachaniBoardPkg/Library/Capsule/CapsuleHookLib/CapsuleHookLib.inf
  !if $(CAPSULE_ENABLE) == TRUE
    CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf
    DisplayUpdateProgressLib|MdeModulePkg/Library/DisplayUpdateProgressLibGraphics/DisplayUpdateProgressLibGraphics.inf
  !else
    CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  !endif
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf

  #
  # CPU
  #
  #VW_DEBUG- LocalApicLib|UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf #VW_DEBUG+
  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf

  HstiLib|MdePkg/Library/DxeHstiLib/DxeHstiLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  TpmCommLib|SecurityPkg/Library/TpmCommLib/TpmCommLib.inf
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  !if $(FTPM_ENABLE) == TRUE
    Tpm2DeviceLib|Override/edk2/SecurityPkg/Library/AmdFtpm/DxeTpm2DeviceLibFsp/Tpm2DeviceLibFtpm.inf
  !else
    Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
  !endif
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  Tcg2PhysicalPresenceLib|Override/edk2/SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.inf
  Tcg2PpVendorLib|SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
  SmmLib|MdePkg/Library/SmmLibNull/SmmLibNull.inf
  ResetSystemLib|MdeModulePkg/Library/DxeResetSystemLib/DxeResetSystemLib.inf
  PostCodeLib|MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
  HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
  BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
  SerialPortLib|VanGoghCommonPkg/Library/BaseSerialPortLib16550AmdFchUart/BaseSerialPortLib16550AmdFchUart.inf

  # Logo
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf

  #
  # For AMD Common
  #
  TimerLib|VanGoghCommonPkg/Library/TscTimerLib/BaseTscTimerLib.inf
  SpiFlashDeviceLib|VanGoghCommonPkg/Library/SpiFlashDeviceLib/SpiFlashDeviceLib.inf

  #
  # For AGESA
  #
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  MicrocodeLib|UefiCpuPkg/Library/MicrocodeLib/MicrocodeLib.inf
  MmUnblockMemoryLib|MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  FspWrapperApiLib|Override/edk2/Fsp2WrapperPkg/Library/BaseFspWrapperApiLib/BaseFspWrapperApiLib.inf
  FspWrapperApiTestLib|Override/edk2/Fsp2WrapperPkg/Library/BaseFspWrapperApiTestLibNull/BaseFspWrapperApiTestLibNull.inf
  FspWrapperPlatformMultiPhaseLib|Override/edk2/Fsp2WrapperPkg/Library/BaseFspWrapperPlatformMultiPhaseLibNull/BaseFspWrapperPlatformMultiPhaseLibNull.inf
  FspWrapperMultiPhaseProcessLib|Override/edk2/Fsp2WrapperPkg/Library/FspWrapperMultiPhaseProcessLib/FspWrapperMultiPhaseProcessLib.inf
  FspWrapperPlatformLib|Override/edk2/Fsp2WrapperPkg/Library/BaseFspWrapperPlatformLibSample/BaseFspWrapperPlatformLibSample.inf
  FspWrapperHobProcessLib|Override/edk2/Fsp2WrapperPkg/Library/PeiFspWrapperHobProcessLibSample/PeiFspWrapperHobProcessLibSample.inf
  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
  AmdIdsHookExtLib|ChachaniBoardPkg/Library/AmdIdsExtLibNull/AmdIdsHookExtLibNull.inf

[LibraryClasses.common.SEC]
  #
  # SEC specific phase
  #
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  !endif
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf

[LibraryClasses.COMMON.PEIM]
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.inf
  PeiResourcePublicationLib|MdePkg/Library/PeiResourcePublicationLib/PeiResourcePublicationLib.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/PeiTcg2PhysicalPresenceLib/PeiTcg2PhysicalPresenceLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  ResetSystemLib|MdeModulePkg/Library/PeiResetSystemLib/PeiResetSystemLib.inf

[LibraryClasses.IA32.PEIM]
  DebugLib | MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

[LibraryClasses.COMMON.PEIM,LibraryClasses.COMMON.PEI_CORE]
  #
  # PEI phase common
  #
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  TimerLib|VanGoghCommonPkg/Library/TscTimerLib/PeiTscTimerLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  !endif
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  SmbusLib|MdePkg/Library/PeiSmbusLibSmbus2Ppi/PeiSmbusLibSmbus2Ppi.inf
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf
  ResetSystemLib|MdeModulePkg/Library/PeiResetSystemLib/PeiResetSystemLib.inf

[LibraryClasses.COMMON.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  !endif

  #
  # Override for AMD Common
  #
  TimerLib|VanGoghCommonPkg/Library/TscTimerLib/DxeTscTimerLib.inf
[LibraryClasses.COMMON.DXE_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|NetworkPkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|NetworkPkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|NetworkPkg/Library/DxeDpcLib/DxeDpcLib.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  !endif

  #
  # Override for AMD Common
  #
  TimerLib|VanGoghCommonPkg/Library/TscTimerLib/DxeTscTimerLib.inf
  SmbiosLib|VanGoghCommonPkg/Library/SmbiosLib/SmbiosLib.inf
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
  FspWrapperMultiPhaseProcessLib|Override/edk2/Fsp2WrapperPkg/Library/DxeFspWrapperMultiPhaseProcessLib/DxeFspWrapperMultiPhaseProcessLib.inf

[LibraryClasses.COMMON.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  !endif
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/SmmTcg2PhysicalPresenceLib/SmmTcg2PhysicalPresenceLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf

[LibraryClasses.COMMON.DXE_RUNTIME_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  !endif
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  !if $(CAPSULE_ENABLE) == TRUE
    CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeRuntimeCapsuleLib.inf
  !endif
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf

  #
  # Override for AMD Common
  #
  TimerLib|VanGoghCommonPkg/Library/TscTimerLib/DxeTscTimerLib.inf

[LibraryClasses.COMMON.UEFI_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  !endif

  #
  # Override for AMD Common
  #
  TimerLib|VanGoghCommonPkg/Library/TscTimerLib/DxeTscTimerLib.inf

[LibraryClasses.COMMON.SMM_CORE]
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
  SmmServicesTableLib|MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf

[LibraryClasses.COMMON.UEFI_APPLICATION]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  !endif
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  #
  # Override for AMD Common
  #
  TimerLib|VanGoghCommonPkg/Library/TscTimerLib/DxeTscTimerLib.inf

[LibraryClasses.X64.PEIM]
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf

[LibraryClasses.common]
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  CcExitLib|UefiCpuPkg/Library/CcExitLibNull/CcExitLibNull.inf
  AmdSvsmLib|UefiCpuPkg/Library/AmdSvsmLibNull/AmdSvsmLibNull.inf

[PcdsFixedAtBuild]
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemBeforePciRestoreSwSmi|0xEA
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemAfterPciRestoreSwSmi|0xEB
  gEfiMdePkgTokenSpaceGuid.PcdPort80DataWidth|32
  gIntelFsp2PkgTokenSpaceGuid.PcdFspHeaderSpecVersion | 0x23 # For FSP-O Header Generation.
  !if $(DISPATCH_BUILD) == FALSE
    gIntelFsp2WrapperTokenSpaceGuid.PcdFspModeSelection|1
  !else
    gIntelFsp2WrapperTokenSpaceGuid.PcdFspModeSelection|0
  !endif
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000 # APCB_TOKEN_UID_DF_BOTTOMIO_VALUE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07 # BIT0-Enable Progress Code(including FPDT)
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|1  # Firmware Performance Data Table(FPDT) Enable
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|64
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeiStackSize|0x80000
  gPlatformPkgTokenSpaceGuid.PcdPeiCorePeiPreMemoryStackBaseAddress|0x30000
  gPlatformPkgTokenSpaceGuid.PcdPeiCorePeiPreMemoryStackSize|0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdExtFpdtBootRecordPadSize|0x20000
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0x08
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x03f8
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|48000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdAriSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMrIovSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"EDK II"
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId|"AMD   "
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x696e616863616863 # "Chachani"
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision|0x0000010
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorId|0x204B4455   #0x55444B20 # 'UDK '
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorRevision|0x0000010
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|0x2060000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizePopulateCapsule|0x2060000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x5000
  gPlatformPkgTokenSpaceGuid.PcdSecureBootDefaultSetting|FALSE
  gUefiCpuPkgTokenSpaceGuid.PcdCpuInitIpiDelayInMicroSeconds|10
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmStackSize|0x8000
  gPlatformPkgTokenSpaceGuid.PcdPciExpressSize|0x10000000 # APCB_TOKEN_UID_DF_PCI_MMIO_SIZE_VALUE
  !if $(DTPM_ENABLE) == TRUE
    gPlatformPkgTokenSpaceGuid.PcdSpiDtpmEnabled|TRUE
  !endif
  gPlatformPkgTokenSpaceGuid.PcdS3AcpiReservedMemorySize|0x200000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"UCC3A27X"
  gEfiSecurityPkgTokenSpaceGuid.PcdTcgPhysicalPresenceInterfaceVer|"1.3"
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2AcpiTableRev|3
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmPlatformClass|0
  gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|8000
  # Make sure both Pcds are set to the same value
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber|128
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout|0
  gPlatformPkgTokenSpaceGuid.PcdFlashFvMainSize|0x180000
  gPlatformPkgTokenSpaceGuid.PcdFlashFvMainUnCompressSize|0x600000
  !if $(CAPSULE_ENABLE) == TRUE
    gAmdCommonPkgTokenSpaceGuid.PcdOtaCapsuleName|L"OtaCapsule.cap"|VOID*|0x40
    gAmdCommonPkgTokenSpaceGuid.PcdOtaCapsulePartitionName|L"capsule"|VOID*|0x40
    ## SHA 256 Hashes of the RSA 2048 public TEST key used to verify Capsule Update images
    # (edk2\BaseTools\Source\Python\Rsa2048Sha256Sign\TestSigningPublicKey.bin)
    # @Prompt The default value is treated as test key. Please do not use default value in the production.
    gEfiSecurityPkgTokenSpaceGuid.PcdRsa2048Sha256PublicKeyBuffer|{0x91, 0x29, 0xc4, 0xbd, 0xea, 0x6d, 0xda, 0xb3, 0xaa, 0x6f, 0x50, 0x16, 0xfc, 0xdb, 0x4b, 0x7e, 0x3c, 0xd6, 0xdc, 0xa4, 0x7a, 0x0e, 0xdd, 0xe6, 0x15, 0x8c, 0x73, 0x96, 0xa2, 0xd4, 0xa6, 0x4d}
  !endif
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0xFEDC9000
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|4
  !if $(TARGET) == RELEASE # Conf/target.txt
    gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  !else
    gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x800000C7
  !endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress|0x0
  gFsp2WrapperTokenSpaceGuid.FspoPeiWorkaroundShadowCopyAddress|0x09E00000
  gEfiMdeModulePkgTokenSpaceGuid.PcdCapsuleOnDiskSupport|TRUE

[PcdsFeatureFlag]
  #gEfiMdeModulePkgTokenSpaceGuid.PcdFrameworkCompatibilitySupport|1
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmEnableBspElection|FALSE #  If enabled, a BSP will be dynamically elected among all processors in each SMI. Otherwise, processor 0 is always as BSP in each SMI.
  !if $(CAPSULE_ENABLE) == TRUE
    gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|TRUE
  !else
    gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|FALSE
  !endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE
[PcdsPatchableInModule]
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

[PcdsDynamicHii.common.DEFAULT]
  gPlatformPkgTokenSpaceGuid.PcdBootState|L"BootState"|gEfiBootStateGuid|0x0|TRUE

[PcdsDynamicDefault]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|3

[PcdsDynamicExDefault]
  # Default Video Resolution
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|0  # 0 - Maximum
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|0    # 0 - Maximum
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0  # 0 - Maximum
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0     # 0 - Maximum
  # Setup Video Resolution
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|0  # 0 - Maximum
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|0    # 0 - Maximum
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutColumn|0  # 0 - Maximum
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutRow|0     # 0 - Maximum
  gEfiSecurityPkgTokenSpaceGuid.PcdUserPhysicalPresence|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x300000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x33E000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x340000
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmBaseAddress|0xFED40000
  gPlatformPkgTokenSpaceGuid.PcdFlashFvMainBase|0x480000
  gPlatformPkgTokenSpaceGuid.PcdFlashFvMainUnCompressBase|0x800000
  # TPM 2.0
  ## GUID value used for PcdTpmInstanceGuid to indicate TPM 2.0 device is selected to support.
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{0x5a, 0xf2, 0x6b, 0x28, 0xc3, 0xc2, 0x8c, 0x40, 0xb3, 0xb4, 0x25, 0xe6, 0x75, 0x8b, 0x73, 0x17}
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2InitializationPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2SelfTestPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2ScrtmPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInitializationPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmScrtmPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2HashMask|0
  gEfiSecurityPkgTokenSpaceGuid.PcdTcg2HashAlgorithmBitmap|0xFFFFFFFF
  !if $(CAPSULE_ENABLE) == TRUE
    gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareImageDescriptor|{0x0}|VOID*|0x100
    gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{GUID("38663FE6-934F-42A1-BCB0-F79E62ECBE80")}|VOID*|0x10
    gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareFileGuid|{GUID("47E20EC0-CFED-47F3-8185-2D0DA2B79897")}|VOID*|0x10
  !endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateDataPtr|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateSmmDataPtr|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdTestKeyUsed|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdGhcbBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdGhcbSize|0
##
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary, FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
##
[Components.IA32]
  UefiCpuPkg/SecCore/SecCore.inf{
    <LibraryClasses>
      PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
      PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
      PlatformSecLib|ChachaniBoardPkg/Library/PlatformSecLib/PlatformSecLib.inf
      NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
  }

  !if $(CAPSULE_ENABLE) == TRUE
    # FMP image decriptor
    ChachaniBoardPkg/Capsule/SystemFirmwareDescriptor/SystemFirmwareDescriptor.inf
  !endif
  #
  # PEI Core
  #
  MdeModulePkg/Core/Pei/PeiMain.inf

  #
  # PEIM
  #
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf
  MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf
  UefiCpuPkg/CpuIoPei/CpuIoPei.inf
  SecurityPkg/Tcg/PhysicalPresencePei/PhysicalPresencePei.inf
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTablePei/FirmwarePerformancePei.inf

  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf  {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  #
  # AMD Platform override
  #
  Override/edk2/UefiCpuPkg/ResetVector/Vtf0/Vtf0.inf

  #
  # AMD Common
  #
  VanGoghCommonPkg/Smm/SmmAccessPei/SmmAccessPei.inf

  # Flash A/B
  VanGoghCommonPkg/Flash_AB/ImageSlotHeader/ImageSlotHeader_1.inf
  VanGoghCommonPkg/Flash_AB/ImageSlotHeader/ImageSlotHeader_2.inf
  VanGoghCommonPkg/Flash_AB/NewEFS/NewEFS.inf
  VanGoghCommonPkg/Flash_AB/PspL1Directory/PspL1Directory.inf

  #
  # Board Specific
  #
  UefiCpuPkg/Universal/Acpi/S3Resume2Pei/S3Resume2Pei.inf
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationPei.inf
  Universal/PlatformInitPei/PlatformInit.inf
  Smm/SmmControlPei/SmmControlPei.inf

  #
  # Trusted Platform Module
  #
  !if $(FTPM_ENABLE) == TRUE || $(DTPM_ENABLE) == TRUE
     Override/edk2/SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigPei.inf {
      <LibraryClasses>
        Tpm2DeviceLib|Override/edk2/SecurityPkg/Library/AmdFtpm/PeiTpm2DeviceLibFsp/Tpm2DeviceLibFtpm.inf
        NULL|Override/edk2/SecurityPkg/Library/AmdFtpm/Tpm2InstanceLibAmdFTpm/Tpm2InstanceLibAmdFTpm.inf
    }
    SecurityPkg/Tcg/Tcg2Pei/Tcg2Pei.inf {
      <LibraryClasses>
        Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterPei.inf
        !if $(DTPM_ENABLE) == TRUE
        NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
        !else
        NULL|Override/edk2/SecurityPkg/Library/AmdFtpm/Tpm2InstanceLibAmdFTpm/Tpm2InstanceLibAmdFTpm.inf
        !endif
        NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
        NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
        NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
    }
  !endif
 Override/edk2/Fsp2WrapperPkg/FspmWrapperPeim/FspmWrapperPeim.inf
  Override/edk2/Fsp2WrapperPkg/FspsWrapperPeim/FspsWrapperPeim.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }
  #Capsule in Memory
  !if $(CAPSULE_ENABLE) == TRUE
    MdeModulePkg/Universal/CapsulePei/CapsulePei.inf
  !endif
[Components.X64]
  #
  # DXE Core
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf
  #Capsule in Memory
  !if $(CAPSULE_ENABLE) == TRUE
    MdeModulePkg/Universal/CapsulePei/CapsuleX64.inf
  !endif
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf  {
    <LibraryClasses>
      !if $(SECURE_BOOT_ENABLE) == TRUE
        NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
      !endif
      NULL|SecurityPkg/Library/DxeImageAuthenticationStatusLib/DxeImageAuthenticationStatusLib.inf
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
  }
  UefiCpuPkg/CpuDxe/CpuDxe.inf
  UefiCpuPkg/CpuS3DataDxe/CpuS3DataDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf

  !if $(VARIABLE_EMULATION) == FALSE
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
  }
  !endif

  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  Override/edk2/Fsp2WrapperPkg/FspsMultiPhaseSiInitDxe/FspsMultiPhaseSiInitDxe.inf

  #
  # Following are the DXE drivers (alphabetical order)
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf {
    <LibraryClasses>
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
      # Board Specific
      PlatformBootManagerLib|ChachaniBoardPkg/Library/Capsule/PlatformBootManagerLib/PlatformBootManagerLib.inf
  }
  MdeModulePkg/Universal/SectionExtractionDxe/SectionExtractionDxe.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterSmm.inf
  MdeModulePkg/Universal/StatusCodeHandler/Smm/StatusCodeHandlerSmm.inf

  MdeModulePkg/Universal/Acpi/BootScriptExecutorDxe/BootScriptExecutorDxe.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  }
  MdeModulePkg/Universal/Acpi/S3SaveStateDxe/S3SaveStateDxe.inf
  MdeModulePkg/Universal/Acpi/SmmS3SaveState/SmmS3SaveState.inf

  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf

  # BGRT table
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf

  #
  # SMM
  #
  MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf
  UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
  MdeModulePkg/Universal/LockBox/SmmLockBox/SmmLockBox.inf
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationSmm.inf
  VanGoghCommonPkg/Smm/AcpiSmm/AcpiSmmPlatform.inf

  #
  # SMBIOS
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

  #
  # PCI
  #
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf

  #
  # NVMe
  #
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  #
  # USB
  #
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf

  #
  # Console
  #
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf{
    <LibraryClasses>
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  }
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf {
    <LibraryClasses>
      CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  }
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/GraphicsOutputDxe/GraphicsOutputDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf

  #
  # File System Modules
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  PcAtChipsetPkg/HpetTimerDxe/HpetTimerDxe.inf
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.inf
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceSmm.inf

  !if $(CAPSULE_ENABLE) == TRUE
    MdeModulePkg/Universal/EsrtDxe/EsrtDxe.inf
    MdeModulePkg/Universal/EsrtFmpDxe/EsrtFmpDxe.inf
    SignedCapsulePkg/Universal/SystemFirmwareUpdate/SystemFirmwareReportDxe.inf {
      <LibraryClasses>
        FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibRsa2048Sha256/FmpAuthenticationLibRsa2048Sha256.inf
    }
    Override/edk2/SignedCapsulePkg/Universal/SystemFirmwareUpdate/SystemFirmwareUpdateDxe.inf {
      <LibraryClasses>
        FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibRsa2048Sha256/FmpAuthenticationLibRsa2048Sha256.inf
    }
  !endif

  #
  # AMD Common
  #
  VanGoghCommonPkg/FvbServices/PlatformSmmSpi.inf
  VanGoghCommonPkg/FlashUpdate/FlashUpdateSmmRuntimeDxe.inf
  VanGoghCommonPkg/FlashUpdate/FlashUpdateSmm.inf
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <LibraryClasses>
      SmmCpuPlatformHookLib|UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
      SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLib.inf
  }
  Override/edk2/PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf

  #
  # Board Specific
  #
  Universal/PlatformSmbiosDxe/PlatformSmbiosDxe.inf
  Universal/AcpiPlatformDxe/AcpiPlatformDxe.inf
  ChachaniBoardPkg/Acpi/AcpiTables/AcpiTables.inf
  Universal/FchSpi/FchSpiRuntimeDxe.inf
  Universal/FchSpi/FchSpiSmm.inf
  Override/edk2/MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
  ChachaniBoardPkg/PciPlatform/PciPlatform.inf
  #
  # Trusted Platform Module
  #
  !if $(FTPM_ENABLE) == TRUE
    SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf {
      <LibraryClasses>
        Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
        NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
        NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
        NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
        NULL|Override/edk2/SecurityPkg/Library/AmdFtpm/Tpm2InstanceLibAmdFTpm/Tpm2InstanceLibAmdFTpm.inf
        PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
    }
    SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf
    Override/edk2/SecurityPkg/Tcg/AmdFtpm/FtpmTcg2Smm/Tcg2Smm.inf
  !endif

  !if $(DTPM_ENABLE) == TRUE
    SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf {
      <LibraryClasses>
        Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
        NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
        NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
        NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
        PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
    }
    SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf {
      <LibraryClasses>
        Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
    }
    Override/edk2/SecurityPkg/Tcg/Tcg2Smm/Tcg2Smm.inf
  !endif

  !if $(FTPM_ENABLE) == TRUE || $(DTPM_ENABLE) == TRUE
    SecurityPkg/Tcg/MemoryOverwriteControl/TcgMor.inf
    SecurityPkg/Tcg/MemoryOverwriteRequestControlLock/TcgMorLockSmm.inf
  !endif

  #
  # Application
  #
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellAcpiViewCommandLib/UefiShellAcpiViewCommandLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|8000
  }

  ShellPkg/DynamicCommand/DpDynamicCommand/DpDynamicCommand.inf{
  <PcdsFixedAtBuild>
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }

  #FSP Wrapper
  Override/edk2/Fsp2WrapperPkg/FspWrapperNotifyDxe/FspWrapperNotifyDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf {
    <LibraryClasses>
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  }

  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf{
    <LibraryClasses>
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  }
  !if $(DISPATCH_BUILD) == FALSE
  # For Bootloader
  Override/edk2/Fsp2WrapperPkg/PrepareForFspSmmDxe/PrepareForFspSmmDxe.inf
  # For FSP During SMM
  Override/edk2/Fsp2WrapperPkg/PrepareForFspSmmDxeFsp/PrepareForFspSmmDxeFsp.inf
  !endif
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf
  VanGoghCommonPkg/Application/UDKFlashUpdate/UDKFlashUpdate.inf
  AmdPlatformPkg/Universal/LogoDxe/S3LogoDxe.inf
[BuildOptions]
  #
  # Define Build Options both for EDK and EDKII drivers.
  #

  # Capsule
  !if $(CAPSULE_ENABLE) == TRUE
    DEFINE MSFT_CAPSULE_SUPPORT_BUILD_OPTION = /DCAPSULE_SUPPORT
    DEFINE  GCC_CAPSULE_SUPPORT_BUILD_OPTION = -DCAPSULE_SUPPORT
  !else
    DEFINE MSFT_CAPSULE_SUPPORT_BUILD_OPTION =
    DEFINE  GCC_CAPSULE_SUPPORT_BUILD_OPTION =
  !endif

  # TPM (fTPM, dTPM)
  !if $(FTPM_ENABLE) == TRUE || $(DTPM_ENABLE) == TRUE
    DEFINE MSFT_TPM_SUPPORT_BUILD_OPTION = /D TPM_ENABLE
    DEFINE  GCC_TPM_SUPPORT_BUILD_OPTION = -DTPM_ENABLE
  !else
    DEFINE MSFT_TPM_SUPPORT_BUILD_OPTION =
    DEFINE  GCC_TPM_SUPPORT_BUILD_OPTION =
  !endif

  # FV_RECOVERY/FV_MAIN combination to memory
  !if $(FV_RECOVERY_MAIN_COMBINE_MEMORY) == TRUE && $(EMULATION_UNCOMPRESSED_FVMAIN) == FALSE
    DEFINE MSFT_FV_RECOVERY_MAIN_COMBINE_MEMORY_BUILD_OPTION = /D FV_RECOVERY_MAIN_COMBINE_SUPPORT
    DEFINE  GCC_FV_RECOVERY_MAIN_COMBINE_MEMORY_BUILD_OPTION = -DFV_RECOVERY_MAIN_COMBINE_SUPPORT
  !else
    DEFINE MSFT_FV_RECOVERY_MAIN_COMBINE_MEMORY_BUILD_OPTION =
    DEFINE  GCC_FV_RECOVERY_MAIN_COMBINE_MEMORY_BUILD_OPTION =
  !endif

  DEFINE MSFT_BOARD_BUILD_OPTIONS = $(MSFT_CAPSULE_SUPPORT_BUILD_OPTION) $(MSFT_TPM_SUPPORT_BUILD_OPTION) $(MSFT_FV_RECOVERY_MAIN_COMBINE_MEMORY_BUILD_OPTION)
  DEFINE  GCC_BOARD_BUILD_OPTIONS = $(GCC_CAPSULE_SUPPORT_BUILD_OPTION) $(GCC_TPM_SUPPORT_BUILD_OPTION) $(GCC_FV_RECOVERY_MAIN_COMBINE_MEMORY_BUILD_OPTION)

  # INTERNAL_IDS
  !if $(INTERNAL_IDS) == YES
    DEFINE MSFT_INTERNAL_IDS_BUILD_OPTION = /D INTERNAL_IDS
    DEFINE  GCC_INTERNAL_IDS_BUILD_OPTION = -DINTERNAL_IDS
  !else
    DEFINE MSFT_INTERNAL_IDS_BUILD_OPTION =
    DEFINE  GCC_INTERNAL_IDS_BUILD_OPTION =
  !endif

  !if $(PERFORMANCE_ENABLE) == TRUE
    DEFINE MSFT_PERFORMANCE_ENABLE_BUILD_OPTIONS =  /D PERFORMANCE_ENABLE
    DEFINE  GCC_PERFORMANCE_ENABLE_BUILD_OPTIONS =  -DPERFORMANCE_ENABLE
  !else
    DEFINE MSFT_PERFORMANCE_ENABLE_BUILD_OPTIONS =
    DEFINE  GCC_PERFORMANCE_ENABLE_BUILD_OPTIONS =
  !endif

  !if $(PRODUCTION_BUILD) == TRUE
    DEFINE MSFT_PRODUCTION_BUILD_OPTIONS = /D PRODUCTION_BUILD
    DEFINE  GCC_PRODUCTION_BUILD_OPTIONS = -DPRODUCTION_BUILD
  !else
    DEFINE MSFT_PRODUCTION_BUILD_OPTIONS =
    DEFINE  GCC_PRODUCTION_BUILD_OPTIONS =
  !endif

  !if $(FSP_SUPPORT_BUILD) == TRUE
    DEFINE MSFT_FSP_SUPPORT_BUILD_OPTIONS = /D USE_EDKII_HEADER_FILE /D DynamicToDynamicEx
    DEFINE  GCC_FSP_SUPPORT_BUILD_OPTIONS = -DUSE_EDKII_HEADER_FILE -DDynamicToDynamicEx
  !else
    DEFINE MSFT_FSP_SUPPORT_BUILD_OPTIONS =
    DEFINE  GCC_FSP_SUPPORT_BUILD_OPTIONS =
  !endif

  DEFINE MSFT_AMD_SPECIFIC_BUILD_OPTIONS = $(MSFT_EMULATION_BUILD_OPTIONS) $(MSFT_INTERNAL_IDS_BUILD_OPTION)
  DEFINE  GCC_AMD_SPECIFIC_BUILD_OPTIONS = $(GCC_EMULATION_BUILD_OPTIONS) $(GCC_INTERNAL_IDS_BUILD_OPTION)

  !if $(COMPRESS_FSP_REGION) == TRUE
    DEFINE MSFT_COMPRESS_FSP_CFLAGS = /D COMPRESS_FSP_REGION
    DEFINE GCC_COMPRESS_FSP_CFLAGS  = -DCOMPRESS_FSP_REGION
  !else
    DEFINE MSFT_COMPRESS_FSP_CFLAGS =
    DEFINE GCC_COMPRESS_FSP_CFLAGS  =
  !endif

[BuildOptions.common.EDKII.DXE_DRIVER,BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER,BuildOptions.common.EDKII.DXE_CORE,BuildOptions.common.EDKII.UEFI_DRIVER, BuildOptions.common.EDKII.UEFI_APPLICATION]
  MSFT:*_*_*_DLINK_FLAGS = /ALIGN:4096
  GCC :*_*_*_DLINK_FLAGS = -z common-page-size=0x1000
  CLANGPDB:*_*_*_DLINK_FLAGS = /ALIGN:4096

[BuildOptions.COMMON.EDKII]
  MSFT:*_*_*_CC_FLAGS = $(MSFT_BOARD_BUILD_OPTIONS) $(MSFT_AMD_SPECIFIC_BUILD_OPTIONS) $(MSFT_PERFORMANCE_ENABLE_BUILD_OPTIONS) $(MSFT_PRODUCTION_BUILD_OPTIONS) $(MSFT_FSP_SUPPORT_BUILD_OPTIONS) $(MSFT_COMPRESS_FSP_CFLAGS)
  MSFT:*_*_*_VFRPP_FLAGS = $(MSFT_INTERNAL_IDS_BUILD_OPTION) $(MSFT_PERFORMANCE_ENABLE_BUILD_OPTIONS) $(MSFT_PRODUCTION_BUILD_OPTIONS)
  GCC :*_*_*_CC_FLAGS = $(GCC_BOARD_BUILD_OPTIONS) $(GCC_AMD_SPECIFIC_BUILD_OPTIONS) $(GCC_PERFORMANCE_ENABLE_BUILD_OPTIONS) $(GCC_PRODUCTION_BUILD_OPTIONS) $(GCC_FSP_SUPPORT_BUILD_OPTIONS) $(GCC_COMPRESS_FSP_CFLAGS)
  GCC :*_*_*_VFRPP_FLAGS = $(GCC_INTERNAL_IDS_BUILD_OPTION) $(GCC_PERFORMANCE_ENABLE_BUILD_OPTIONS) $(GCC_PRODUCTION_BUILD_OPTIONS)
