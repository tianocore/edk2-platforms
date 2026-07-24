## @file
#  The main build description file for the NordBoardPkg (Qualcomm Nord Generic).
#
#  MinPlatform-based UEFI platform for the (Oryon) Nord Generic, built to run
#  as the TF-A non-trusted firmware (BL33).
#
#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DSC_SPECIFICATION           = 0x0001001E
  PLATFORM_GUID               = 5bfe3924-e072-4f08-bf15-94c4293d8242
  PLATFORM_NAME               = NordBoardPkg
  PLATFORM_VERSION            = 0.1
  SUPPORTED_ARCHITECTURES     = AARCH64
  FLASH_DEFINITION            = Platform/Qualcomm/NordBoardPkg/NordBoardPkg.fdf
  OUTPUT_DIRECTORY            = Build/NordBoardPkg
  BUILD_TARGETS               = DEBUG | RELEASE | NOOPT
  SKUID_IDENTIFIER            = DEFAULT
  SMM_REQUIRED                = FALSE

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE USER_PROVIDED_DTB    = FALSE

[SkuIds]
  0 | DEFAULT

[PcdsFixedAtBuild]
  ######################################
  # MinPlatform Stage
  ######################################
  # Stage 1 - enable debug (system deadloop after debug init)
  # Stage 2 - mem init (system deadloop after mem init)
  # Stage 3 - boot to shell only
  # Stage 4 - boot to OS
  #
  gMinPlatformPkgTokenSpaceGuid.PcdBootStage | 4

#
# MinPlatform common include for required feature PCD. These PCD must be set
# before the core include files, CoreCommonLib, CorePeiLib, and CoreDxeLib.
#
!include MinPlatformPkg/Include/Dsc/MinPlatformFeaturesPcd.dsc.inc

[PcdsFixedAtBuild]
  # The firmware device is loaded by TF-A as BL33 at this DRAM address; the
  # MinPlatform flash-area PCDs describe the FD image layout at that base.
  gMinPlatformPkgTokenSpaceGuid.PcdFlashAreaBaseAddress   | 0xA0200000

  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel        | 0x80000002
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel   | 0x80000002
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask           | 0x2f

[PcdsFeatureFlag]
  gMinPlatformPkgTokenSpaceGuid.PcdSerialTerminalEnable   | TRUE
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemEnable         | TRUE
  gQualcommPlatformPkgTokenSpaceGuid.PcdUfsHcEnable       | TRUE

# Include common libraries, then stage-specific libraries and components.
!include MinPlatformPkg/Include/Dsc/CoreCommonLib.dsc
!include MinPlatformPkg/Include/Dsc/CorePeiLib.dsc
!include MinPlatformPkg/Include/Dsc/CoreDxeLib.dsc
!include QualcommMinPlatformPkg/Include/Dsc/Stage1.dsc.inc
!include QualcommMinPlatformPkg/Include/Dsc/Stage2.dsc.inc
!include QualcommMinPlatformPkg/Include/Dsc/Stage3.dsc.inc
!include QualcommMinPlatformPkg/Include/Dsc/Stage4.dsc.inc

#
# Board-specific stage content (extends the shared QualcommMinPlatformPkg
# Stage3/Stage4 with Nord's boot-manager, storage discovery and SMBIOS set).
#
[LibraryClasses.Common]
  PlatformBootManagerLib  | ArmPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf

[Components.Common]
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
  }
  MdeModulePkg/Universal/DriverHealthManagerDxe/DriverHealthManagerDxe.inf
  MdeModulePkg/Universal/Disk/UdfDxe/UdfDxe.inf

  #
  # Connect-all boot discovery (UFS/SCSI/FAT enumerated at boot).
  #
  MdeModulePkg/Universal/BootManagerPolicyDxe/BootManagerPolicyDxe.inf

  MdeModulePkg/Logo/LogoDxe.inf
  MdeModulePkg/Universal/Disk/RamDiskDxe/RamDiskDxe.inf

  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib | ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL | ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellAcpiViewCommandLib/UefiShellAcpiViewCommandLib.inf
      HandleParsingLib | ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      PrintLib | MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib | ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask | 0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize | FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize | 8000
  }

  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

[Components.AARCH64]
  #
  # Generic non-discoverable PCI used for many SoC devices (e.g. UFS).
  #
  MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf

  #
  # SMBIOS support
  #
  ArmPkg/Universal/Smbios/ProcessorSubClassDxe/ProcessorSubClassDxe.inf
  ArmPkg/Universal/Smbios/SmbiosMiscDxe/SmbiosMiscDxe.inf

#
# Qualcomm Silicon and Platform dsc includes (provide GENI SerialPortLib,
# ArmPlatformLib, RamPartitionTableLib, ReportCpuHobLib, the SEC/PEIM library
# overrides, and the shared Qualcomm DXE components).
#
!include Silicon/Qualcomm/QualcommSiliconPkg/QualcommSiliconPkg.dsc.inc
!include Platform/Qualcomm/QualcommPlatformPkg/QualcommPlatformPkg.dsc.inc

[LibraryClasses.AARCH64]
  PeCoffExtraActionLib|ArmPkg/Library/DebugPeCoffExtraActionLib/DebugPeCoffExtraActionLib.inf

[LibraryClasses.common]
  # Qualcomm real SMEM library.
  SmemLib|Silicon/Qualcomm/QualcommSiliconPkg/Library/SmemLib/SmemLib.inf

  # SMBIOS platform information (board-specific).
  OemMiscLib|Platform/Qualcomm/QualcommPlatformPkg/Library/OemMiscLib/OemMiscLib.inf

  # MinPlatform board hooks.
  BoardInitLib|Platform/Qualcomm/NordBoardPkg/Library/BoardInitLib/BoardInitLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf

  # ARM / platform libraries not provided by the MinPlatform core includes.
  ArmSmcLib|MdePkg/Library/ArmSmcLib/ArmSmcLib.inf
  ArmHvcLib|ArmPkg/Library/ArmHvcLib/ArmHvcLib.inf
  ArmMonitorLib|ArmPkg/Library/ArmMonitorLib/ArmMonitorLib.inf
  ArmGenericTimerCounterLib|ArmPkg/Library/ArmGenericTimerVirtCounterLib/ArmGenericTimerVirtCounterLib.inf
  DmaLib|EmbeddedPkg/Library/NonCoherentDmaLib/NonCoherentDmaLib.inf
  TimeBaseLib|EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf
  RealTimeClockLib|EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf
  ArmTransferListLib|ArmPkg/Library/ArmTransferListLib/ArmTransferListLib.inf

[LibraryClasses.common.DXE_CORE]
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf

[LibraryClasses.common.DXE_DRIVER]
  NonDiscoverableDeviceRegistrationLib|MdeModulePkg/Library/NonDiscoverableDeviceRegistrationLib/NonDiscoverableDeviceRegistrationLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

[LibraryClasses.common.UEFI_DRIVER]
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

[LibraryClasses.common.UEFI_APPLICATION]
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

#
# Qualcomm platform PCD overrides for the Nord Generic.
#
[PcdsFixedAtBuild]
  # SystemMemory describes the UEFI working region (== FD base), not the whole
  # DRAM bank; upper DRAM is discovered from the SMEM RAM partition table by
  # ArmPlatformLib (AddUpperMemoryFromRamPartitions) during PEI. PEI installs
  # permanent memory at the top (SystemMemoryTop - PcdSystemMemoryUefiRegionSize).
  gArmTokenSpaceGuid.PcdSystemMemoryBase                  | 0xA0200000
  gArmTokenSpaceGuid.PcdSystemMemorySize                  | 0x10000000
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize| 0x02000000

  gArmPlatformTokenSpaceGuid.PcdCoreCount                 | 8
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase         | 0xA0A00000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize   | 0x80000

  # SMEM region for the QualcommSiliconPkg SmemLib (Nord SMEM base).
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemBaseAddress    | 0x89B00000
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemSize           | 0x200000

  # Cap RAM-partition-derived memory at 64 GB (matches the board).
  gQualcommPlatformPkgTokenSpaceGuid.PcdMaxMemory         | 0x1000000000

  # Memory protection: the QualcommPlatformPkg ArmPlatformLib installs a coarse
  # static page-table map, so disable DXE image / NX protection (the DXE core
  # faults applying per-section attributes to its own image otherwise).
  gEfiMdeModulePkgTokenSpaceGuid.PcdImageProtectionPolicy        | 0x00000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeNxMemoryProtectionPolicy  | 0x0000000000000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetNxForStack                | FALSE

  # PEI stack ceiling for the MinPlatform PEI flow.
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeiStackSize | 0x90000

  # Qcom GENI serial console.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase    | 0x884000
  # 0-PCANSI 1-VT100 2-VT00+ 3-UTF8 4-TTYTERM
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType         | 4

  # UFS register window: mapped by ArmPlatformLib, used by QcomUfsHcDxe as the BAR.
  gQualcommPlatformPkgTokenSpaceGuid.PcdUfsHcMmioBase      | 0x1D44000
  gQualcommPlatformPkgTokenSpaceGuid.PcdUfsHcMmioSize      | 0x3000

  # BDS boot-manager menu app. The MdeModulePkg default points at the standalone
  # BootManagerMenuApp (EEC25BDC-...), which Nord does not build; UiApp provides the
  # menu instead. Point PcdBootManagerMenuFile at UiApp's GUID (462CAA21-...) so
  # EfiBootManagerGetBootManagerMenu() finds it (else PlatformBm.c ASSERTs).
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile    | { 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

  # SMBIOS strings.
  gArmTokenSpaceGuid.PcdSystemProductName                 | L"Qualcomm Nord Generic"
  gArmTokenSpaceGuid.PcdSystemVersion                     | L"1.0"
  gArmTokenSpaceGuid.PcdBaseBoardManufacturer             | L"Qualcomm"
  gArmTokenSpaceGuid.PcdBaseBoardProductName              | L"Nord Generic"
  gArmTokenSpaceGuid.PcdBaseBoardVersion                  | L"1.0"
  gArmTokenSpaceGuid.PcdProcessorManufacturer             | L"Qualcomm"
  gArmTokenSpaceGuid.PcdProcessorVersion                  | L"qcom,oryon"
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor        | L"Qualcomm Technologies, Inc."
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString | L"1.0"

[PcdsDynamicDefault]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut         | 3

  # Boot discovery policy: connect ALL devices (UFS/SCSI/FAT) at boot.
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootDiscoveryPolicy   | 2

  # ARM Generic Interrupt Controller (GICv3).
  gArmTokenSpaceGuid.PcdGicDistributorBase                | 0x17000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase             | 0x17080000

  # SMBIOS entry point version.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion         | 0x0307

#
# Board-specific components not provided by the stage includes or the
# Qualcomm dsc.inc files.
#
[Components.AARCH64]
  #
  # UFS host controllers (Qualcomm GENI + PCI UFS).
  #
  Silicon/Qualcomm/Drivers/QcomUfsHcDxe/QcomUfsHcDxe.inf
  MdeModulePkg/Bus/Pci/UfsPciHcDxe/UfsPciHcDxe.inf

  #
  # Device tree: build the placeholder DummyDeviceTree unless the user supplies
  # a board DTB with -D USER_PROVIDED_DTB=TRUE. DtPlatformDxe installs it as the
  # EFI DT configuration table.
  #
!if $(USER_PROVIDED_DTB) == FALSE
  Platform/Qualcomm/NordBoardPkg/DeviceTree/DummyDeviceTree.inf
!endif
  EmbeddedPkg/Drivers/DtPlatformDxe/DtPlatformDxe.inf {
    <LibraryClasses>
      DtPlatformDtbLoaderLib|EmbeddedPkg/Library/DxeDtPlatformDtbLoaderLibDefault/DxeDtPlatformDtbLoaderLibDefault.inf
  }

