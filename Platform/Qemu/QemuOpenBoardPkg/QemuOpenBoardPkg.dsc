## @file
#  QemuOpenBoardPkg.dsc
#
#  Description file for QemuOpenBoardPkg
#
#  Copyright (c) 2022 Theo Jehl
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DSC_SPECIFICATION           = 0x0001001C
  PLATFORM_GUID               = 94797875-D562-40CF-8D55-ADD623C8D46C
  PLATFORM_NAME               = QemuOpenBoardPkg
  PLATFORM_VERSION            = 0.1
  SUPPORTED_ARCHITECTURES     = IA32 | X64
  FLASH_DEFINITION            = $(PLATFORM_NAME)/$(PLATFORM_NAME).fdf
  OUTPUT_DIRECTORY            = Build/$(PLATFORM_NAME)
  BUILD_TARGETS               = DEBUG | RELEASE | NOOPT
  SKUID_IDENTIFIER            = ALL
  SMM_REQUIRED                = FALSE

!ifndef $(PEI_ARCH)
  !error "PEI_ARCH must be specified to build this feature!"
!endif
!ifndef $(DXE_ARCH)
  !error "DXE_ARCH must be specified to build this feature!"
!endif

[SkuIds]
  0 | DEFAULT

[Packages]
  MdePkg/MdePkg.dec
  MdeModulePkg/MdeModulePkg.dec
  MinPlatformPkg/MinPlatformPkg.dec
  QemuOpenBoardPkg/QemuOpenBoardPkg.dec
  UefiCpuPkg/UefiCpuPkg.dec
  OvmfPkg/OvmfPkg.dec

[PcdsFixedAtBuild]
  ######################################
  # Key Boot Stage and FSP configuration
  ######################################
  #
  # Please select the Boot Stage here.
  # Stage 1 - enable debug (system deadloop after debug init)
  # Stage 2 - mem init (system deadloop after mem init)
  # Stage 3 - boot to shell only
  # Stage 4 - boot to OS
  # Stage 5 - boot to OS with security boot enabled
  # Stage 6 - boot with advanced features enabled
  #
  gMinPlatformPkgTokenSpaceGuid.PcdBootStage | 4

#
# MinPlatform common include for required feature PCD
# These PCD must be set before the core include files, CoreCommonLib,
# CorePeiLib, and CoreDxeLib.
# Optional MinPlatformPkg features should be enabled after this
#
!include MinPlatformPkg/Include/Dsc/MinPlatformFeaturesPcd.dsc.inc

#
# Commonly used MinPlatform feature configuration logic that maps functionity to stage
#
!include BoardModulePkg/Include/Dsc/CommonStageConfig.dsc.inc

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel                      | 0x802A00C7
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel                 | 0x802A00C7
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask                         | 0x17

  # QEMU "memory" is functional even in SEC.  For simplicity, we just use that
  # "memory" for the temporary RAM
  gQemuOpenBoardPkgTokenSpaceGuid.PcdTemporaryRamBase                   | 0x1000000
  gQemuOpenBoardPkgTokenSpaceGuid.PcdTemporaryRamSize                   | 0x010000

  gQemuOpenBoardPkgTokenSpaceGuid.PcdDebugIoPort                        | 0x402
  gEfiMdePkgTokenSpaceGuid.PcdFSBClock                                  | 100000000

  # PCIe base address for Q35 machines
  # QEMU usable memory below 4G cannot exceed 2.8Gb
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress                     | 0xB0000000

  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable             | TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange  | FALSE

  gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMBase                      | 0x00000000 # Will be updated by build

[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportUefiDecompress         | TRUE

  !if $(DXE_ARCH) == X64
    gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode            | TRUE
  !else
    gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode            | FALSE
  !endif

  gMinPlatformPkgTokenSpaceGuid.PcdSerialTerminalEnable                 | TRUE

  !if $(SMM_REQUIRED) == TRUE
    gUefiOvmfPkgTokenSpaceGuid.PcdSmmSmramRequire                       | TRUE
    gUefiCpuPkgTokenSpaceGuid.PcdCpuHotPlugSupport                      | FALSE
    gEfiMdeModulePkgTokenSpaceGuid.PcdEnableVariableRuntimeCache        | FALSE
  !endif

[PcdsDynamicDefault]
  gUefiOvmfPkgTokenSpaceGuid.PcdOvmfHostBridgePciDevId                  | 0

  # Video setup
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution      | 640
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution        | 480

  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion                       | 0x0208
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev                        | 0x0

  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut                       | 3

  gUefiCpuPkgTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber             | 0
  gUefiCpuPkgTokenSpaceGuid.PcdCpuBootLogicalProcessorNumber            | 0

  !if $(SMM_REQUIRED) == TRUE
    gUefiOvmfPkgTokenSpaceGuid.PcdQ35TsegMbytes                         | 8
    gUefiOvmfPkgTokenSpaceGuid.PcdQ35SmramAtDefaultSmbase               | FALSE
    gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmSyncMode                         | 0x01
    gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmApSyncTimeout                    | 100000
  !endif

# Include Common libraries and then stage specific libraries and components
!include MinPlatformPkg/Include/Dsc/CoreCommonLib.dsc
!include MinPlatformPkg/Include/Dsc/CorePeiLib.dsc
!include MinPlatformPkg/Include/Dsc/CoreDxeLib.dsc
!include QemuOpenBoardPkg/Include/Dsc/Stage1.dsc.inc
!include QemuOpenBoardPkg/Include/Dsc/Stage2.dsc.inc
!include QemuOpenBoardPkg/Include/Dsc/Stage3.dsc.inc
!include QemuOpenBoardPkg/Include/Dsc/Stage4.dsc.inc

[LibraryClasses.Common]
  QemuOpenFwCfgLib        | QemuOpenBoardPkg/Library/QemuOpenFwCfgLib/QemuOpenFwCfgLib.inf
  PlatformHookLib         | MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  PlatformSecLib          | QemuOpenBoardPkg/Library/PlatformSecLib/PlatformSecLib.inf
  DebugLib                | MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  PciCf8Lib               | MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  TimerLib                | OvmfPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf
  AmdSvsmLib|UefiCpuPkg/Library/AmdSvsmLibNull/AmdSvsmLibNull.inf

[LibraryClasses.Common.DXE_CORE]
  TimerLib                | OvmfPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf

[LibraryClasses.Common.DXE_DRIVER, LibraryClasses.Common.DXE_RUNTIME_DRIVER, LibraryClasses.Common.DXE_SMM_DRIVER, LibraryClasses.Common.UEFI_DRIVER, LibraryClasses.Common.UEFI_APPLICATION, LibraryClasses.Common.SMM_CORE]
  TimerLib                | OvmfPkg/Library/AcpiTimerLib/DxeAcpiTimerLib.inf
  AcpiPlatformLib         | OvmfPkg/Library/AcpiPlatformLib/DxeAcpiPlatformLib.inf
  QemuFwCfgLib            | OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgDxeLib.inf
  QemuFwCfgSimpleParserLib| OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf
  MemEncryptSevLib        | OvmfPkg/Library/BaseMemEncryptSevLib/DxeMemEncryptSevLib.inf
  MemEncryptTdxLib        | OvmfPkg/Library/BaseMemEncryptTdxLib/BaseMemEncryptTdxLibNull.inf
  Tcg2PhysicalPresenceLib | OvmfPkg/Library/Tcg2PhysicalPresenceLibNull/DxeTcg2PhysicalPresenceLib.inf
  ResetSystemLib          | OvmfPkg/Library/ResetSystemLib/DxeResetSystemLib.inf
  PlatformBootManagerCommonLib | OvmfPkg/Library/PlatformBootManagerCommonLib/PlatformBootManagerCommonLib.inf

[LibraryClasses.Common.SEC]
  DebugLib                | OvmfPkg/Library/PlatformDebugLibIoPort/PlatformRomDebugLibIoPort.inf

[Components.$(DXE_ARCH)]
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
