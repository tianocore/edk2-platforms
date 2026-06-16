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
!include GlymurOpenBoardPkg/Include/Dsc/Stage1.dsc.inc
!include GlymurOpenBoardPkg/Include/Dsc/Stage2.dsc.inc
!include GlymurOpenBoardPkg/Include/Dsc/Stage3.dsc.inc
!include GlymurOpenBoardPkg/Include/Dsc/Stage4.dsc.inc

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
