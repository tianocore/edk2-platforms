#
#  Copyright (c) 2021-2026, Marvell International All rights reserved.
#  Copyright (c) 2014-2016, Cavium Inc. All rights reserved.
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#  Copyright (c) 2015, Intel Corporation. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent

[Defines]
  SECURE_BOOT_ENABLE  = FALSE
  DEFINE CUSTOM_STACK_CHECK_LIB = STATIC

!include NetworkPkg/NetworkDefines.dsc.inc

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_ARM_DLINK_FLAGS = -z common-page-size=0x1000
  GCC:*_*_AARCH64_DLINK_FLAGS = -z common-page-size=0x10000

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf

  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf

  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

  #
  # Assume everything is fixed at build
  #
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

!if $(NETWORK_TLS_ENABLE) == TRUE
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
!else
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
!endif

  # ARM Architectural Libraries
  ArmLib|MdePkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|UefiCpuPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
  ArmSmcLib|MdePkg/Library/ArmSmcLib/ArmSmcLib.inf
  ArmMonitorLib|ArmPkg/Library/ArmMonitorLib/ArmMonitorLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  ArmGenericTimerCounterLib|ArmPkg/Library/ArmGenericTimerPhyCounterLib/ArmGenericTimerPhyCounterLib.inf
  PL011UartLib|ArmPlatformPkg/Library/PL011UartLib/PL011UartLib.inf

  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf

  # USB Requirements
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf

  # CN10K specific libraries
  CN10KConfigLib|Silicon/Marvell/CN10K/Library/CN10KConfigLib/CN10KConfigLib.inf

  # Boot manager
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf

  # ARM PL011 UART Runtime Library
  PL011UartClockLib|ArmPlatformPkg/Library/PL011UartClockLib/PL011UartClockLib.inf
  SerialPortLib|ArmPlatformPkg/Library/PL011SerialPortLib/PL011SerialPortLib.inf

  PeCoffExtraActionLib|ArmPkg/Library/DebugPeCoffExtraActionLib/DebugPeCoffExtraActionLib.inf

  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  DebugAgentTimerLib|EmbeddedPkg/Library/DebugAgentTimerLibNull/DebugAgentTimerLibNull.inf

  FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf

  # RunAxf support via Dynamic Shell Command protocol
  # It uses the Shell libraries.
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  # Used by FmpDxe and the network stack
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

  #
  # Secure Boot dependencies
  #
!if $(SECURE_BOOT_ENABLE) == TRUE
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
  # re-use the UserPhysicalPresent() dummy implementation from the ovmf tree
  PlatformSecureLib|OvmfPkg/Library/PlatformSecureLib/PlatformSecureLib.inf
!else
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif

  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf

  # Platform Support Libraries
  SmcLib|Silicon/Marvell/Library/SmcLib/SmcLib.inf

[LibraryClasses.common.SEC]
  HobLib|EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
  PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
  ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
  PeilessSecMeasureLib|SecurityPkg/Library/PeilessSecMeasureLib/PeilessSecMeasureLibNull.inf
  MemoryAllocationLib|EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf
  PrePiHobListPointerLib|ArmPlatformPkg/Library/PrePiHobListPointerLib/PrePiHobListPointerLib.inf
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
  MemoryInitPeiLib|ArmPlatformPkg/MemoryInitPei/MemoryInitPeiLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeRuntimeCapsuleLib.inf

[LibraryClasses.AARCH64.DXE_RUNTIME_DRIVER]
  #
  # PSCI support in EL3 may not be available if we are not running under a PSCI
  # compliant secure firmware, but since the default VExpress EfiResetSystemLib
  # cannot be supported at runtime (due to the fact that the syscfg MMIO registers
  # cannot be runtime remapped), it is our best bet to get ResetSystem functionality
  # on these platforms.
  #
  ResetSystemLib|ArmPkg/Library/ArmPsciResetSystemLib/ArmPsciResetSystemLib.inf

[BuildOptions]
  RVCT:RELEASE_*_*_CC_FLAGS  = -DMDEPKG_NDEBUG

  GCC:RELEASE_*_*_CC_FLAGS  = -DMDEPKG_NDEBUG

# GCC will generate code that runs on processors as idicated by -march
# Single = (append) allows flags appendixes coming from [BuildOptions] defined in specific INFs.
!if $(SIGNED_CAPSULE) == 1
  GCC:*_*_AARCH64_PLATFORM_FLAGS = -march=armv8.2-a -DSIGNED_CAPSULE
!else
  GCC:*_*_AARCH64_PLATFORM_FLAGS = -march=armv8.2-a
!endif

!if $(SMC_DIMM_HANDLE) == 1
  GCC:*_*_AARCH64_PLATFORM_FLAGS = -DSMC_DIMM_HANDLE
!endif

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  gEmbeddedTokenSpaceGuid.PcdPrePiProduceMemoryTypeInformationHob|TRUE
  gMarvellSiliconTokenSpaceGuid.PcdFixupFdt|FALSE

[PcdsFixedAtBuild.common]
!ifdef $(FIRMWARE_VER)
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"$(FIRMWARE_VER)"
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"MARVELL UEFI 5.0.0"
!endif

!ifdef $(RELEASE_DATE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareReleaseDateString|L"$(RELEASE_DATE)"
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareReleaseDateString|L"2026"
!endif

  # DEBUG_ASSERT_ENABLED       0x01
  # DEBUG_PRINT_ENABLED        0x02
  # DEBUG_CODE_ENABLED         0x04
  # CLEAR_MEMORY_ENABLED       0x08
  # ASSERT_BREAKPOINT_ENABLED  0x10
  # ASSERT_DEADLOOP_ENABLED    0x20
!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x21
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f
!endif

  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07

  # RunAxf support via Dynamic Shell Command protocol
  # We want to use the Shell Libraries but don't want it to initialise
  # automatically. We initialise the libraries when the command is called by the
  # Shell.
  gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE

  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

!if $(SECURE_BOOT_ENABLE) == TRUE
  # override the default values from SecurityPkg to ensure images from all sources are verified in secure boot
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04
!endif

  # Stacks for MPCores in Normal World
  # Non-Trusted DRAM
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x2E000000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000

  # System Memory (40 - 1TB of DRAM)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x00004000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x10000000000

  # Size of the region used by UEFI in permanent memory (Reserved 32MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x02000000

  ## PL011 - Serial Terminal
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x87e028000000
  gArmPlatformTokenSpaceGuid.PL011UartClkInHz|100000000

  #
  # ARM General Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x801000000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x801000080000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x801000020000

  gMarvellSiliconTokenSpaceGuid.PcdFdtConfigRootNode|"/cavium,bdk"

[Components.common]
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  ArmPkg/Drivers/ArmPsciMpServicesDxe/ArmPsciMpServicesDxe.inf

  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  #
  # EFI_RNG_PROTOCOL producer. Required by NetworkPkg/DxeNetLib (and hence by
  # every network driver), which has a depex on gEfiRngProtocolGuid.
  #
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf {
    <LibraryClasses>
      ArmTrngLib|ArmPkg/Library/ArmTrngLib/ArmTrngLib.inf
  }

  #
  # Networking stack
  #
!include NetworkPkg/Network.dsc.inc

  # FV Filesystem
  MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf

  # Regexp
  MdeModulePkg/Universal/RegularExpressionDxe/RegularExpressionDxe.inf

  # SectionExtraction
  MdeModulePkg/Universal/SectionExtractionDxe/SectionExtractionDxe.inf {
    <LibraryClasses>
      ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  }

  #
  # FDT support
  #
  Silicon/Marvell/Drivers/Fdt/FdtPlatformDxe/FdtPlatformDxe.inf
  Silicon/Marvell/Drivers/Fdt/FdtClientDxe/FdtClientDxe.inf

  #
  # EMMC/SD
  #
  MdeModulePkg/Bus/Sd/SdDxe/SdDxe.inf
  MdeModulePkg/Bus/Sd/EmmcDxe/EmmcDxe.inf
  MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf
  Silicon/Marvell/Drivers/SdMmc/CN10KMmcDxe/CN10KMmcDxe.inf {
    <LibraryClasses>
      CadenceSd6PhyLib|Silicon/Cadence/CadenceSiliconPkg/Library/CadenceSd6PhyLib/CadenceSd6PhyLib.inf
      NonDiscoverableDeviceRegistrationLib|MdeModulePkg/Library/NonDiscoverableDeviceRegistrationLib/NonDiscoverableDeviceRegistrationLib.inf
  }
  MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.inf

  #
  # UEFI application (Shell Embedded Boot Loader)
  #
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellAcpiViewCommandLib/UefiShellAcpiViewCommandLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork2CommandsLib/UefiShellNetwork2CommandsLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
      Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  }

  # EBC Support
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf

  ShellPkg/DynamicCommand/DpDynamicCommand/DpDynamicCommand.inf
  ShellPkg/DynamicCommand/TftpDynamicCommand/TftpDynamicCommand.inf

  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf {
    <LibraryClasses>
      CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  }
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf {
    <LibraryClasses>
      BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
      CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf
      DisplayUpdateProgressLib|MdeModulePkg/Library/DisplayUpdateProgressLibText/DisplayUpdateProgressLibText.inf
      PlatformBootManagerLib|ArmPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  }
  MdeModulePkg/Universal/BootManagerPolicyDxe/BootManagerPolicyDxe.inf

  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

[Components.common]
  #
  # SEC Phase modules
  #
  # UEFI is placed in RAM by bootloader
  ArmPlatformPkg/PeilessSec/PeilessSec.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
      ArmTransferListLib|MdePkg/Library/ArmTransferListLib/ArmTransferListLib.inf
      ArmPlatformLib|Silicon/Marvell/CN10K/Library/CN10KSysLib/CN10KSysLib.inf
      PlatformPeiLib|ArmPlatformPkg/PlatformPei/PlatformPeiLib.inf
  }

  #
  # DXE
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }

!if $(DEBUG) == 1
  #
  # Status codes
  #
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
!endif

  #
  # Architectural Protocols
  #
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
  }
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!else
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
!endif
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  }

  # Platform DXE
  Silicon/Marvell/CN10K/Drivers/PlatformDxe/PlatformDxe.inf

  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  Silicon/Marvell/Drivers/Wdt/GtiWatchdogDxe/GtiWatchdogDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf {
    <LibraryClasses>
      # No PL031 on CN10K.
      RealTimeClockLib|EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf
      TimeBaseLib|EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf
  }
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf

  #
  # ACPI Support
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  Silicon/Marvell/CN10K/Drivers/AcpiPlatformDxe/AcpiPlatformDxe.inf {
    <LibraryClasses>
      AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf
      AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  }
  Silicon/Marvell/CN10K/AcpiTables/AcpiTables.inf

  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.inf {
    <LibraryClasses>
      LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  }

  #
  # ARM Support
  #
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf

  #
  # NOR Flash Support
  #
  Silicon/Marvell/Drivers/Spi/CN10KSpiDxe/CN10KSpiDxe.inf {
    <LibraryClasses>
      CadenceXspiLib|Silicon/Cadence/CadenceSiliconPkg/Library/CadenceXspiLib/CadenceXspiLib.inf
      NorFlashInfoLib|EmbeddedPkg/Library/NorFlashInfoLib/NorFlashInfoLib.inf
  }
  Silicon/Marvell/Drivers/Spi/MvFvbDxe/MvFvbDxe.inf

  #
  # SMBIOS Support
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  Silicon/Marvell/CN10K/Drivers/SmbiosPlatformDxe/SmbiosPlatformDxe.inf

  #
  # FAT filesystem + GPT/MBR partitioning
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf {
    <LibraryClasses>
      GptLib|MdeModulePkg/Library/GptLib/GptLib.inf
  }
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  #
  # PCI Support
  #
  ArmPkg/Drivers/ArmPciCpuIo2Dxe/ArmPciCpuIo2Dxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      PciSegmentInfoLib|Silicon/Marvell/CN10K/Library/CN10KPciLib/CN10KPciLib.inf
      PciSegmentLib|MdePkg/Library/PciSegmentLibSegmentInfo/BasePciSegmentLibSegmentInfo.inf
      PciHostBridgeLib|Silicon/Marvell/CN10K/Library/CN10KPciLib/CN10KPciLib.inf
  }
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf

  #
  # SATA Controller
  #
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf

  #
  # SCSI Support
  #
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  #
  # Usb Support
  #
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  #
  # NVME
  #
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  #
  # Capsule App
  #
  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf

  #
  # Firmware Management Protocol
  #
  MdeModulePkg/Universal/EsrtDxe/EsrtDxe.inf
  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      FILE_GUID = E3B2F1A0-6C4D-4B8E-9F1A-2D7C5E8B3A61
    <LibraryClasses>
      FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
      FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
      FmpDependencyLib|FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf
      FmpDependencyCheckLib|FmpDevicePkg/Library/FmpDependencyCheckLibNull/FmpDependencyCheckLibNull.inf
      FmpDependencyDeviceLib|FmpDevicePkg/Library/FmpDependencyDeviceLibNull/FmpDependencyDeviceLibNull.inf
      CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
      FmpDeviceLib|Silicon/Marvell/CN10K/Library/CN10KFmpDeviceLib/CN10KFmpDeviceLib.inf
    <PcdsFixedAtBuild>
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"CN10K System Firmware"
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|0x0
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressColor|0x00FF00FF
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressWatchdogTimeInSeconds|4
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceLockEventGuid|{GUID(gEfiEventExitBootServicesGuid)}
  }

  #
  # Config UI
  #
  Silicon/Marvell/CN10K/Drivers/ConfigUIDxe/ConfigUIDxe.inf
