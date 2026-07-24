## @file
#  The main build description file for the GlymurOpenBoard.
#
#  Copyright (c) 2022 Theo Jehl<BR>
#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DSC_SPECIFICATION           = 0x0001001E
  PLATFORM_GUID               = 416EE676-95FE-4CF7-9CEC-DCDB255C9FB1
  PLATFORM_NAME               = GlymurOpenBoardPkg
  PLATFORM_VERSION            = 1.0
  SUPPORTED_ARCHITECTURES     = AARCH64
  FLASH_DEFINITION            = $(PLATFORM_NAME)/$(PLATFORM_NAME).fdf
  OUTPUT_DIRECTORY            = Build/$(PLATFORM_NAME)
  BUILD_TARGETS               = DEBUG | RELEASE | NOOPT
  SKUID_IDENTIFIER            = ALL
  SMM_REQUIRED                = FALSE

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
  # Stage 5 - boot to OS with security boot enabled
  # Stage 6 - boot with advanced features enabled
  #
  gMinPlatformPkgTokenSpaceGuid.PcdBootStage | 3

#
# MinPlatform common include for required feature PCD
# These PCD must be set before the core include files, CoreCommonLib,
# CorePeiLib, and CoreDxeLib.
# Optional MinPlatformPkg features should be enabled after this
#
!include MinPlatformPkg/Include/Dsc/MinPlatformFeaturesPcd.dsc.inc

[PcdsFixedAtBuild]
  gMinPlatformPkgTokenSpaceGuid.PcdFlashAreaBaseAddress                   | 0xA7000000
  gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMBase                        | 0x00000000 # Will be updated by build

  #
  # gEfiMdePkgTokenSpaceGuid Overrides
  #
  !ifdef FULL_VERBOSE_LOG
    gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel                      | 0x802A00C7
    gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel                 | 0x802A00C7
  !else
    gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel                      | 0x80000006
    gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel                 | 0x80000006
  !endif

  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask                           | 0x17
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType                         | 0x4

  #
  # gEfiMdeModulePkgTokenSpaceGuid Overrides
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile                   | { 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable               | TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange    | FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase                    | 0x00894000
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeiStackSize                | 0x90000

[PcdsFeatureFlag]
  gMinPlatformPkgTokenSpaceGuid.PcdSerialTerminalEnable                   | TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportUefiDecompress           | TRUE
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemEnable                         | TRUE
  gQualcommPlatformPkgTokenSpaceGuid.PcdBootDtEnable                      | TRUE
  gQualcommPlatformPkgTokenSpaceGuid.PcdIMemCookiesEnable                 | TRUE
  gQualcommPlatformPkgTokenSpaceGuid.PcdTrace32Enable                     | TRUE

[PcdsDynamicDefault]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut                         | 3

  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion                         | 0x0208
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev                          | 0x0

  gUefiCpuPkgTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber               | 0
  gUefiCpuPkgTokenSpaceGuid.PcdCpuBootLogicalProcessorNumber              | 0

# Include Common libraries and then stage specific libraries and components
!include MinPlatformPkg/Include/Dsc/CoreCommonLib.dsc
!include MinPlatformPkg/Include/Dsc/CorePeiLib.dsc
!include MinPlatformPkg/Include/Dsc/CoreDxeLib.dsc
!include QualcommMinPlatformPkg/Include/Dsc/Stage1.dsc.inc
!include QualcommMinPlatformPkg/Include/Dsc/Stage2.dsc.inc
!include QualcommMinPlatformPkg/Include/Dsc/Stage3.dsc.inc
!include QualcommMinPlatformPkg/Include/Dsc/Stage4.dsc.inc

#
# Board-specific stage content (extends the shared QualcommMinPlatformPkg
# Stage3/Stage4 with Glymur's PCI/ISA/USB/storage driver set).
#
[LibraryClasses.Common]
  PlatformBootManagerLib  | MdeModulePkg/Library/PlatformBootManagerLibNull/PlatformBootManagerLibNull.inf
  IoLib                   | MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciExpressLib           | MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  DebugLib                | MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

[LibraryClasses.AARCH64]
  IoLib                    | MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsicArmVirt.inf
  ArmGenericTimerCounterLib| ArmPkg/Library/ArmGenericTimerPhyCounterLib/ArmGenericTimerPhyCounterLib.inf
  PlatformBootManagerLib   | ArmPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf

[Components.Common]
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Universal/Console/GraphicsOutputDxe/GraphicsOutputDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  MdeModulePkg/Bus/Isa/IsaBusDxe/IsaBusDxe.inf
  MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KeyboardDxe.inf
  MdeModulePkg/Bus/Ufs/UfsPassThruDxe/UfsPassThruDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  PcAtChipsetPkg/Bus/Pci/IdeControllerDxe/IdeControllerDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf

  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib | ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL | ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
      HandleParsingLib | ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      PrintLib | MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib | ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask | 0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize | FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize | 8000
  }

  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
      FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootDiscoveryPolicyUiLib/BootDiscoveryPolicyUiLib.inf
  }
  MdeModulePkg/Bus/Pci/PciSioSerialDxe/PciSioSerialDxe.inf

[LibraryClasses.Common.DXE_SMM_DRIVER]
  LockBoxLib              | MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf

[Components.AARCH64]
  MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf

#
# Qualcomm Silicon and Platform dsc
#
!include Silicon/Qualcomm/QualcommSiliconPkg/QualcommSiliconPkg.dsc.inc
!include Platform/Qualcomm/QualcommPlatformPkg/QualcommPlatformPkg.dsc.inc

#
# Qualcomm Platform Override for this target
#
[PcdsFixedAtBuild]
  #
  # gArmTokenSpaceGuid Overrides
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase                                  | 0xA7000000
  gArmTokenSpaceGuid.PcdSystemMemorySize                                  | 0x10000000

  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum                            | 29
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum                               | 30

  gArmTokenSpaceGuid.PcdGicDistributorBase                                | 0x17000000      # APSS_GICD_CTLR
  gArmTokenSpaceGuid.PcdGicRedistributorsBase                             | 0x17080000      # APSS_GICR0_CTLR

  gArmTokenSpaceGuid.PcdUefiShellDefaultBootEnable                        | TRUE

  #
  # gArmPlatformTokenSpaceGuid Overrides
  #
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize                | 0x02000000

  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase                         | gArmTokenSpaceGuid.PcdSystemMemoryBase + 0x400000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize                   | 0x80000

  #
  # gQualcommSiliconPkgTokenSpaceGuid Overrides
  #

  # SMEM
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemBaseAddress                    | 0xFFE00000
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemSize                           | 0x200000
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemMaxItems                       | 512
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemTcsrBase                       | 0x1f00000
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemMutexRegBase                   | 0x00040000
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemWonceRegBase                   | 0x000c0000
  gQualcommSiliconPkgTokenSpaceGuid.PcdSmemTargetInfoWonceReg             | 0x00000000

  #
  # gQualcommPlatformPkgTokenSpaceGuid Overrides
  #

  # Device Tree (DT)
  gQualcommPlatformPkgTokenSpaceGuid.PcdBootDtBase                        | 0xA9000000
  gQualcommPlatformPkgTokenSpaceGuid.PcdBootDtSize                        | 0x00070000

  # Internal Memory (IMEM) Cookie
  gQualcommPlatformPkgTokenSpaceGuid.PcdIMemCookiesBase                   | 0x14680000
  gQualcommPlatformPkgTokenSpaceGuid.PcdIMemCookiesSize                   | 0x00001000

  # Trace32 Debug
  gQualcommPlatformPkgTokenSpaceGuid.PcdTrace32DdrBase                    | 0xA8FFB000
  gQualcommPlatformPkgTokenSpaceGuid.PcdTrace32DdrSize                    | 0x00005000

[LibraryClasses.Common]
  BoardInitLib            | GlymurOpenBoardPkg/Library/BoardInitLib/BoardInitLib.inf
  PlatformHookLib         | MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
