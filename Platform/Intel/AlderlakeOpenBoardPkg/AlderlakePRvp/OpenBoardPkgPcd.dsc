## @file
#  Board description file initializes configuration (PCD) settings for the project.
#
#   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
#   SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Pcd Section - list of all PCD Entries used by this board.
#
################################################################################

[PcdsFixedAtBuild.common]
  ######################################
  # Key Boot Stage
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
  gMinPlatformPkgTokenSpaceGuid.PcdBootStage|5

  ######################################
  # Platform Configuration
  ######################################
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

[PcdsFixedAtBuild.common]
  ######################################
  # FSP configuration
  ######################################
  #
  # 0: FSP Wrapper is running in Dispatch mode.
  # 1: FSP Wrapper is running in API mode.
  #
  gIntelFsp2WrapperTokenSpaceGuid.PcdFspModeSelection|0

  #
  # FALSE: The board is not a FSP wrapper (FSP binary not used)
  # TRUE:  The board is a FSP wrapper (FSP binary is used)
  #
  gMinPlatformPkgTokenSpaceGuid.PcdFspWrapperBootMode|TRUE

  #
  # FSP Base address PCD will be updated in FDF basing on flash map.
  #
  gIntelFsp2WrapperTokenSpaceGuid.PcdFsptBaseAddress|0
  gIntelFsp2WrapperTokenSpaceGuid.PcdFspmBaseAddress|0

  gIntelFsp2PkgTokenSpaceGuid.PcdTemporaryRamBase|0xFEF00000
  gIntelFsp2PkgTokenSpaceGuid.PcdTemporaryRamSize|0x00100000
  gSiPkgTokenSpaceGuid.PcdTemporaryRamBase|0xFEF80000
  gSiPkgTokenSpaceGuid.PcdTemporaryRamSize|0x00040000
  gSiPkgTokenSpaceGuid.PcdTsegSize|0x1000000


  ######################################
  # Silicon Configuration
  ######################################
  gSiPkgTokenSpaceGuid.PcdAdlLpSupport|TRUE
  gSiPkgTokenSpaceGuid.PcdOptimizeCompilerEnable|TRUE
  gSiPkgTokenSpaceGuid.PcdSourceDebugEnable|FALSE
  gSiPkgTokenSpaceGuid.PcdAcpiEnable|FALSE
  gSiPkgTokenSpaceGuid.PcdSmbiosEnable|TRUE
  gSiPkgTokenSpaceGuid.PcdFspWrapperEnable|TRUE
  gSiPkgTokenSpaceGuid.PcdSerialIoUartEnable|TRUE
  gSiPkgTokenSpaceGuid.PcdPeiDisplayEnable|TRUE

  #
  # PCD declared for Fru
  #
  gSiPkgTokenSpaceGuid.PcdMrcTraceMessageSupported|TRUE
  gSiPkgTokenSpaceGuid.PcdSmmVariableEnable|TRUE
  gSiPkgTokenSpaceGuid.PcdIgdEnable|TRUE

#
# 16 MB BIOS Flash Base Address and Size
#
  gSiPkgTokenSpaceGuid.PcdBiosAreaBaseAddress|0xFF000000
  gSiPkgTokenSpaceGuid.PcdBiosSize|0x01000000
  gSiPkgTokenSpaceGuid.PcdBdatEnable|TRUE
  gSiPkgTokenSpaceGuid.PcdSiCatalogDebugEnable|FALSE

  #
  # When sharing stack with boot loader, FSP only needs a small temp ram for heap
  #
!if gIntelFsp2WrapperTokenSpaceGuid.PcdFspModeSelection == 1
  gIntelFsp2PkgTokenSpaceGuid.PcdFspTemporaryRamSize|0x00020000
!else
      #
      # FSP Dispatch mode will not establish separate Stack or Heap.
      #
  gIntelFsp2PkgTokenSpaceGuid.PcdFspTemporaryRamSize|0
!endif

  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xC0000000
  gIntelSiliconPkgTokenSpaceGuid.PcdAcpiBaseAddress|0x1800

  gBoardModuleTokenSpaceGuid.PcdDefaultBoardId|0x13

  gPlatformModuleTokenSpaceGuid.PcdLzmaEnable|TRUE
  #
  # BIOS build switches configuration
  #
  gPlatformModuleTokenSpaceGuid.PcdGmAdrAddress|(gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress - 0x10000000) # 0xB0000000
  gPlatformModuleTokenSpaceGuid.PcdGttMmAddress|(gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress - 0x11000000) # 0xAF000000

  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeiStackSize|0x80000

  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciDeviceNumber|0x1F
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciFunctionNumber|0x2
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciEnableRegisterOffset|0x44
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoBarEnableMask|0x80
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciBarRegisterOffset|0x00
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPortBaseAddress|0x1800
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiPm1TmrOffset|0x08
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPortBaseAddressMask|0xFFFC

  ## Specifies the size of the microcode Region.
  # @Prompt Microcode Region size.
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMicrocodePatchRegionSize|0

#
# NvStorage Base Address and Size
#
  gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageOffset|0x00000000
  gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageSize|0x00060000

  #
  # Set the location of the DUTY_CYCLE field in the P_CNT register
  # and indicate the width of the clock duty cycle to OS power management
  #
  gMinPlatformPkgTokenSpaceGuid.PcdFadtDutyOffset|0x1
  gMinPlatformPkgTokenSpaceGuid.PcdFadtDutyWidth|0x3

[PcdsFeatureFlag.common]
  ######################################
  # Edk2 Configuration
  ######################################
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreImageLoaderSearchTeSectionFirst|FALSE
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmEnableBspElection|FALSE
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmProfileEnable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE

  ######################################
  # Board Configuration
  ######################################
  gBoardModuleTokenSpaceGuid.PcdIntelGopEnable|TRUE

[PcdsFixedAtBuild.common]
  ######################################
  # Edk2 Configuration
  ######################################
!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x3
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2F
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
!endif

  #
  # INIT is now triggered before BIOS by ucode/hardware. In the OVMF
  # environment, QEMU lacks a simulation for the INIT process.
  # To address this, PcdFirstTimeWakeUpAPsBySipi set to FALSE to
  # broadcast INIT-SIPI-SIPI for the first time.
  #
  gUefiCpuPkgTokenSpaceGuid.PcdFirstTimeWakeUpAPsBySipi|FALSE

!if $(TARGET) == DEBUG
  !if gSiPkgTokenSpaceGuid.PcdSerialIoUartEnable == TRUE
    gSiPkgTokenSpaceGuid.PcdSerialIoUartDebugEnable|1
    gPlatformModuleTokenSpaceGuid.PcdStatusCodeUseSerialIoUart|TRUE
  !endif
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdBrowserFieldTextColor|0x01
  gEfiMdeModulePkgTokenSpaceGuid.PcdBrowserSubtitleTextColor|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdHwErrStorageSize|0x00000800
  gEfiMdeModulePkgTokenSpaceGuid.PcdLoadModuleAtFixAddressEnable|$(TOP_MEMORY_ADDRESS)
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdReclaimVariableSpaceAtEndOfDxe|TRUE

  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoBarEnableMask|0x80
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciBarRegisterOffset|0x00
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciBusNumber|0x0
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciDeviceNumber|0x1F
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciEnableRegisterOffset|0x44
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciFunctionNumber|0x2
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPortBaseAddress|0x1800
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPortBaseAddressMask|0xFFFC
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiPm1TmrOffset|0x08

  gMinPlatformPkgTokenSpaceGuid.PcdAcpiResetRegisterAccessSize|0x1
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPm1aEvtBlkAccessSize|0x2
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPm1bEvtBlkAccessSize|0x2
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPm1aCntBlkAccessSize|0x2
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPm1bCntBlkAccessSize|0x2

  gUefiCpuPkgTokenSpaceGuid.PcdCpuNumberOfReservedVariableMtrrs|0x0
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmApSyncTimeout|10000
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmStackSize|0x20000

  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPm2CntBlkAccessSize|0x1
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPmTmrBlkAccessSize|0x2
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXGpe0BlkAccessSize|0x1
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXGpe1BlkAccessSize|0x1

  gSiPkgTokenSpaceGuid.PcdSerialIoUartNumber|0



  gMinPlatformPkgTokenSpaceGuid.PcdPciReservedMemBase|0x80400000
  gMinPlatformPkgTokenSpaceGuid.PcdPciReservedMemLimit|0xBFFFFFFF
  #
  # The PCDs are used to control the Windows SMM Security Mitigations Table - Protection Flags
  #
  # BIT0: If set, expresses that for all synchronous SMM entries,SMM will validate that input and output buffers lie entirely within the expected fixed memory regions.
  # BIT1: If set, expresses that for all synchronous SMM entries, SMM will validate that input and output pointers embedded within the fixed communication buffer only refer to address ranges \
  #       that lie entirely within the expected fixed memory regions.
  # BIT2: Firmware setting this bit is an indication that it will not allow reconfiguration of system resources via non-architectural mechanisms.
  # BIT3-31: Reserved
  #
  gMinPlatformPkgTokenSpaceGuid.PcdWsmtProtectionFlags|0x07

  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiAcpiReclaimMemorySize|0xCC
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiAcpiNvsMemorySize|0xA2
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiReservedMemorySize|0x3100
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiRtDataMemorySize|0x2A
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiRtCodeMemorySize|0xC4

!if gSiPkgTokenSpaceGuid.PcdAdlLpSupport == TRUE
  gSiPkgTokenSpaceGuid.PcdAdlSSupport|FALSE
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootStage == 1
  gMinPlatformPkgTokenSpaceGuid.PcdTestPointIbvPlatformFeature|{0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootStage == 2
  gMinPlatformPkgTokenSpaceGuid.PcdTestPointIbvPlatformFeature|{0x03, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootStage == 3
  gMinPlatformPkgTokenSpaceGuid.PcdTestPointIbvPlatformFeature|{0x03, 0x07, 0x03, 0x05, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootStage == 4
  gMinPlatformPkgTokenSpaceGuid.PcdTestPointIbvPlatformFeature|{0x03, 0x07, 0x03, 0x05, 0x1F, 0x00, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootStage == 5
  gMinPlatformPkgTokenSpaceGuid.PcdTestPointIbvPlatformFeature|{0x03, 0x0F, 0x07, 0x1F, 0x1F, 0x0F, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 6
  gMinPlatformPkgTokenSpaceGuid.PcdTestPointIbvPlatformFeature|{0x03, 0x0F, 0x07, 0x1F, 0x1F, 0x0F, 0x0F, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
!endif

[PcdsFixedAtBuild.IA32]
  gIntelFsp2WrapperTokenSpaceGuid.PcdPeiMinMemSize|0x3800000
  gMinPlatformPkgTokenSpaceGuid.PcdPeiPhaseStackTop|0xA0000

[PcdsFixedAtBuild.X64]
  ######################################
  # Edk2 Configuration
  ######################################

  # Default platform supported RFC 4646 languages: (American) English
  gEfiMdePkgTokenSpaceGuid.PcdUefiVariableDefaultPlatformLangCodes|"en-US"

[PcdsPatchableInModule.common]
  ######################################
  # Edk2 Configuration
  ######################################
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0304

  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046



[PcdsDynamicDefault]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x20202020204C4349

  gMinPlatformPkgTokenSpaceGuid.PcdPciReservedMemAbove4GBBase|0xFFFFFFFFFFFFFFFF
  gMinPlatformPkgTokenSpaceGuid.PcdPciReservedMemAbove4GBLimit|0x0000000000000000
  gMinPlatformPkgTokenSpaceGuid.PcdPciSegmentCount|0x1


  gSiPkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x20202020204C4349

gIntelFsp2WrapperTokenSpaceGuid.PcdFspsBaseAddress|0xFFD20000
gIntelFsp2WrapperTokenSpaceGuid.PcdFspmUpdDataAddress|0x00000000
gIntelFsp2WrapperTokenSpaceGuid.PcdFspsUpdDataAddress|0x00000000

!if $(TARGET) == DEBUG
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|TRUE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
!endif

  ## Specifies max supported number of Logical Processors.
  # @Prompt Configure max supported number of Logical Processorss
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber|16
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE

!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInitializationPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{0x5a, 0xf2, 0x6b, 0x28, 0xc3, 0xc2, 0x8c, 0x40, 0xb3, 0xb4, 0x25, 0xe6, 0x75, 0x8b, 0x73, 0x17}
!endif

[PcdsDynamicHii.common.DEFAULT]
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2AcpiTableRev|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x8|4|NV,BS

[PcdsDynamicHii.X64.DEFAULT]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5 # Variable: L"Timeout"
  gEfiMdePkgTokenSpaceGuid.PcdHardwareErrorRecordLevel|L"HwErrRecSupport"|gEfiGlobalVariableGuid|0x0|1 # Variable: L"HwErrRecSupport"

[PcdsDynamicExDefault]

  gMinPlatformPkgTokenSpaceGuid.PcdPciExpressRegionLength|0x10000000
  #
  # Some of the PCD consumed by both FSP and bootloader should be defined
  # here for bootloader to consume.
  #

  gBoardModuleTokenSpaceGuid.PcdBoardGpioTablePreMem|{0x0}
  gBoardModuleTokenSpaceGuid.PcdBoardGpioTablePreMem.GpioConfig[60].GpioPad|0x0
  gBoardModuleTokenSpaceGuid.PcdBoardGpioTable|{0x0}
  gBoardModuleTokenSpaceGuid.PcdBoardGpioTable.GpioConfig[130].GpioPad|0x0
  gBoardModuleTokenSpaceGuid.PcdBoardGpioTableEarlyPreMem|{0x0}
  gBoardModuleTokenSpaceGuid.PcdBoardGpioTableEarlyPreMem.GpioConfig[40].GpioPad|0x0
  gBoardModuleTokenSpaceGuid.PcdBoardAcpiData|{0x0}

 #
 # Include FSP PCD settings.
 #
 !include $(PLATFORM_FSP_BIN_PACKAGE)/FspPkgPcdShare.dsc

#
# FSP Binary base address will be set in FDF basing on flash map
#
gIntelFsp2WrapperTokenSpaceGuid.PcdFspsBaseAddress|0

  !include $(PLATFORM_BOARD_PACKAGE)/SBCVpdStructurePcd/AllStructPCD.dsc

[PcdsDynamicExVpd.common.DEFAULT]
  gBoardModuleTokenSpaceGuid.VpdPcdBoardGpioTablePreMem| * |{CODE({
    {0x0}  // terminator
  })}

  gBoardModuleTokenSpaceGuid.VpdPcdBoardGpioTable| * |{CODE({
    {0x0}  // terminator
  })}

  gBoardModuleTokenSpaceGuid.VpdPcdPcieClkUsageMap| * |{CODE(
    {0x0}
  )}

  gBoardModuleTokenSpaceGuid.VpdPcdMrcSpdData| * |{CODE(
    {0x0}
  )}

  gBoardModuleTokenSpaceGuid.VpdPcdMrcDqsMapCpu2Dram| * |{CODE(
    {0x0}
  )}

  gBoardModuleTokenSpaceGuid.VpdPcdMrcDqMapCpu2Dram| * |{CODE(
    {0x0}
  )}
