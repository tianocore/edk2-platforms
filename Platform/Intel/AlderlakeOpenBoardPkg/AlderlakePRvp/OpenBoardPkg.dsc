## @file
#  The main build description file for the AlderlakePRvp board.
#
#   Copyright (c) 2022 - 2024, Intel Corporation. All rights reserved.<BR>
#   SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
  #
  # Set platform specific package/folder name, same as passed from PREBUILD script.
  # PLATFORM_PACKAGE would be the same as PLATFORM_NAME as well as package build folder
  # DEFINE only takes effect at R9 DSC and FDF.
  #
  DEFINE      PLATFORM_PACKAGE                = MinPlatformPkg
  DEFINE      PLATFORM_SI_PACKAGE             = AlderlakeSiliconPkg
  DEFINE      PLATFORM_SI_BIN_PACKAGE         = AlderlakeSiliconBinPkg
  DEFINE      PLATFORM_FSP_BIN_PACKAGE        = RaptorLakeFspBinPkg/Client/RaptorLakeP
  DEFINE      PLATFORM_BOARD_PACKAGE          = AlderlakeOpenBoardPkg
  DEFINE      BOARD                           = AlderlakePRvp
  DEFINE      PROJECT                         = $(PLATFORM_BOARD_PACKAGE)/$(BOARD)
  #
  # Default value for OpenBoardPkg.fdf use
  #
  DEFINE BIOS_SIZE_OPTION = SIZE_160

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                       = $(PLATFORM_BOARD_PACKAGE)
  PLATFORM_GUID                       = EB89E595-7D9D-4422-A277-A50B5AFD3E16
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  OUTPUT_DIRECTORY                    = Build/$(PROJECT)
  SUPPORTED_ARCHITECTURES             = IA32|X64
  BUILD_TARGETS                       = DEBUG|RELEASE
  SKUID_IDENTIFIER                    = ALL


  FLASH_DEFINITION                    = $(PROJECT)/OpenBoardPkg.fdf

  VPD_TOOL_GUID                       = 8C3D856A-9BE6-468E-850A-24F7A8D38E08
  FIX_LOAD_TOP_MEMORY_ADDRESS         = 0x0
  DEFINE   TOP_MEMORY_ADDRESS         = 0x0

  #
  # Include PCD configuration for this board
  #
  !include OpenBoardPkgPcd.dsc
  !include AdvancedFeaturePkg/Include/AdvancedFeaturesPcd.dsc

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]                                           # SkuId = PcdBoardBomId << 16 | PcdBoardRev << 8 | PcdBoardId
  0|DEFAULT                                        # The entry: 0|DEFAULT is reserved and always required.
  0x000012|SkuIdAdlPDdr5Rvp

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include $(PLATFORM_PACKAGE)/Include/Dsc/CoreCommonLib.dsc
!include $(PLATFORM_PACKAGE)/Include/Dsc/CorePeiLib.dsc
!include $(PLATFORM_PACKAGE)/Include/Dsc/CoreDxeLib.dsc

[LibraryClasses.common]
  PeiLib|$(PLATFORM_PACKAGE)/Library/PeiLib/PeiLib.inf
  ReportFvLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/PeiReportFvLib/PeiReportFvLib.inf

  PciHostBridgeLib|$(PLATFORM_PACKAGE)/Pci/Library/PciHostBridgeLibSimple/PciHostBridgeLibSimple.inf
  PciSegmentInfoLib|$(PLATFORM_PACKAGE)/Pci/Library/PciSegmentInfoLibSimple/PciSegmentInfoLibSimple.inf
  PlatformHookLib|$(PLATFORM_BOARD_PACKAGE)/Library/BasePlatformHookLib/BasePlatformHookLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf

  SiliconPolicyInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/SiliconPolicyInitLibNull/SiliconPolicyInitLibNull.inf
  SiliconPolicyUpdateLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/SiliconPolicyUpdateLibNull/SiliconPolicyUpdateLibNull.inf

#
# Platform
#

  FspCommonLib|IntelFsp2Pkg/Library/BaseFspCommonLib/BaseFspCommonLib.inf
  FspWrapperApiLib|IntelFsp2WrapperPkg/Library/BaseFspWrapperApiLib/BaseFspWrapperApiLib.inf
  FspWrapperApiTestLib|IntelFsp2WrapperPkg/Library/PeiFspWrapperApiTestLib/PeiFspWrapperApiTestLib.inf
  FspSwitchStackLib|IntelFsp2Pkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf

  ConfigBlockLib|IntelSiliconPkg/Library/BaseConfigBlockLib/BaseConfigBlockLib.inf

  BoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/BoardInitLibNull/BoardInitLibNull.inf
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf

  PciSegmentLib|$(PLATFORM_SI_PACKAGE)/Library/BasePciSegmentMultiSegLibPci/BasePciSegmentMultiSegLibPci.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf

  PostCodeMapLib|PostCodeDebugFeaturePkg/Library/PostCodeMapLib/PostCodeMapLib.inf

  PlatformSecLib|$(PLATFORM_BOARD_PACKAGE)/FspWrapper/Library/SecFspWrapperPlatformSecLib/SecFspWrapperPlatformSecLib.inf
  FspWrapperPlatformLib|$(PLATFORM_PACKAGE)/FspWrapper/Library/PeiFspWrapperPlatformLib/PeiFspWrapperPlatformLib.inf
  FspWrapperHobProcessLib|$(PLATFORM_PACKAGE)/FspWrapper/Library/PeiFspWrapperHobProcessLib/PeiFspWrapperHobProcessLib.inf

#
# Platform
#
    PlatformBootManagerLib|$(PLATFORM_PACKAGE)/Bds/Library/DxePlatformBootManagerLib/DxePlatformBootManagerLib.inf
    BoardBootManagerLib|BoardModulePkg/Library/BoardBootManagerLib/BoardBootManagerLib.inf
    BoardBdsHookLib|BoardModulePkg/Library/BoardBdsHookLib/BoardBdsHookLib.inf

#
# Silicon Init Package
#
!include $(PLATFORM_SI_PACKAGE)/Product/Alderlake/SiPkgCommonLib.dsc

#
# Features
#

  ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
  HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
  BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf

[LibraryClasses.common.PEIM]
  FirmwareBootMediaInfoLib|BoardModulePkg/Library/PeiFirmwareBootMediaInfoLib/PeiFirmwareBootMediaInfoLib.inf

[LibraryClasses.IA32]
#
# PEI phase common
#

  ResetSystemLib|MdeModulePkg/Library/PeiResetSystemLib/PeiResetSystemLib.inf


!if $(TARGET) == DEBUG
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/PeiTestPointCheckLib.inf
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf
!endif
  TestPointLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointLib/PeiTestPointLib.inf
  MultiBoardInitSupportLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/PeiMultiBoardInitSupportLib.inf
  BoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/PeiMultiBoardInitSupportLib.inf

  BoardConfigLib|$(PLATFORM_BOARD_PACKAGE)/Library/PeiBoardConfigLib/PeiBoardConfigLib.inf

#
# Silicon Init Package
#
!include $(PLATFORM_SI_PACKAGE)/Product/Alderlake/SiPkgPeiLib.dsc

#
# Features
#
  FirmwareBootMediaLib|IntelSiliconPkg/Library/PeiDxeSmmBootMediaLib/PeiFirmwareBootMediaLib.inf
  Usb3DebugPortLib|Usb3DebugFeaturePkg/Library/Usb3DebugPortLib/Usb3DebugPortLibNull.inf

#
# SmmAccess
#
  SmmAccessLib|IntelSiliconPkg/Feature/SmmAccess/Library/PeiSmmAccessLib/PeiSmmAccessLib.inf

  PeiGetFvInfoLib|$(PLATFORM_BOARD_PACKAGE)/Library/PeiGetFvInfoLib/PeiGetFvInfoLib.inf
  PeiPolicyUpdateLib|$(PLATFORM_BOARD_PACKAGE)/Policy/Library/PeiPolicyUpdateLib/PeiPolicyUpdateLib.inf

[LibraryClasses.IA32.SEC]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  Usb3DebugPortLib|Usb3DebugFeaturePkg/Library/Usb3DebugPortLib/Usb3DebugPortLibNull.inf
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/SecTestPointCheckLib.inf
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf
  ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  SecBoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/SecBoardInitLibNull/SecBoardInitLibNull.inf

[LibraryClasses.IA32.PEIM]

  FspSwitchStackLib|IntelFsp2Pkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf


  #
  # Use Null library instance to skip MTRR initialization from MinPlatformPkg PlatformInit modules.
  # MTRR configuration will be done by FSP or OpenBoardPlatformInit modules.
  #
  SetCacheMtrrLib|$(PLATFORM_PACKAGE)/Library/SetCacheMtrrLib/SetCacheMtrrLibNull.inf

  #
  # This is library instance of Cpu Hob Library, replacing the library instance from MinPlatform.
  #
  ReportCpuHobLib|$(PLATFORM_SI_PACKAGE)/Fru/AdlCpu/PeiReportCpuHob/Library/PeiReportCpuHobLib/PeiReportCpuHobLib.inf

!if $(TARGET) == DEBUG
  #
  # This is for reducing NATIVE DEBUG binary size, replacing some library routines with PPI.
  #
  DebugLib|MdeModulePkg/Library/PeiDebugLibDebugPpi/PeiDebugLibDebugPpi.inf
!endif


[LibraryClasses.X64]
  #
  # DXE phase common
  #

  FspWrapperPlatformLib|$(PLATFORM_PACKAGE)/FspWrapper/Library/DxeFspWrapperPlatformLib/DxeFspWrapperPlatformLib.inf


!if $(TARGET) == DEBUG
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/DxeTestPointCheckLib.inf
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf
!endif
  TestPointLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointLib/DxeTestPointLib.inf
  MultiBoardInitSupportLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/DxeMultiBoardInitSupportLib.inf
  BoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/DxeMultiBoardInitSupportLib.inf
  MultiBoardAcpiSupportLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/DxeMultiBoardAcpiSupportLib.inf
  BoardAcpiTableLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/DxeMultiBoardAcpiSupportLib.inf
  SiliconPolicyUpdateLib|$(PLATFORM_BOARD_PACKAGE)/Policy/Library/DxeSiliconPolicyUpdateLib/DxeSiliconPolicyUpdateLib.inf
  TcgPhysicalPresenceLib|SecurityPkg/Library/DxeTcgPhysicalPresenceLib/DxeTcgPhysicalPresenceLib.inf
  TcgPpVendorLib|SecurityPkg/Library/TcgPpVendorLibNull/TcgPpVendorLibNull.inf

#
# Silicon Init Package
#
!include $(PLATFORM_SI_PACKAGE)/Product/Alderlake/SiPkgDxeLib.dsc

 FirmwareBootMediaLib|IntelSiliconPkg/Library/PeiDxeSmmBootMediaLib/DxeSmmFirmwareBootMediaLib.inf
 Usb3DebugPortLib|Usb3DebugFeaturePkg/Library/Usb3DebugPortLib/Usb3DebugPortLibNull.inf


  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf

[LibraryClasses.X64.DXE_SMM_DRIVER]

  SpiFlashCommonLib|IntelSiliconPkg/Library/SmmSpiFlashCommonLib/SmmSpiFlashCommonLib.inf

!if $(TARGET) == DEBUG
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/SmmTestPointCheckLib.inf
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf
!endif
  TestPointLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointLib/SmmTestPointLib.inf
  MultiBoardAcpiSupportLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/SmmMultiBoardAcpiSupportLib.inf
  BoardAcpiEnableLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/SmmMultiBoardAcpiSupportLib.inf

[LibraryClasses.X64.DXE_RUNTIME_DRIVER]

  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf


ResetSystemLib|$(PLATFORM_SI_PACKAGE)/Pch/Library/BaseResetSystemLib/BaseResetSystemLib.inf

[Components.IA32]

#
# Common
#
!include $(PLATFORM_PACKAGE)/Include/Dsc/CorePeiInclude.dsc

  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      !if $(TARGET) == DEBUG
        DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
      !endif
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/SectionExtractionPei/SectionExtractionPei.inf {
    <LibraryClasses>
      NULL|SecurityPkg/Library/PeiRsa2048Sha256GuidedSectionExtractLib/PeiRsa2048Sha256GuidedSectionExtractLib.inf
  }

  #
  # FSP wrapper SEC Core
  #
  UefiCpuPkg/SecCore/SecCore.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
      NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
  }


#
# CpuFeatures PEIM
#
  UefiCpuPkg/CpuFeatures/CpuFeaturesPei.inf {
    <LibraryClasses>
      RegisterCpuFeaturesLib|UefiCpuPkg/Library/RegisterCpuFeaturesLib/PeiRegisterCpuFeaturesLib.inf
      NULL|UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  }
#
# This is for reducing NATIVE DEBUG binary size, replacing some library routines with PPI.
#
!if $(TARGET) == DEBUG
  MdeModulePkg/Universal/DebugServicePei/DebugServicePei.inf {
    <LibraryClasses>
      DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  }
!endif

#
# Platform
#
  $(PLATFORM_PACKAGE)/PlatformInit/ReportFv/ReportFvPei.inf
  $(PLATFORM_PACKAGE)/PlatformInit/PlatformInitPei/PlatformInitPreMem.inf {
    <LibraryClasses>
      NULL|$(PROJECT)/Library/BoardInitLib/Pei/PeiMultiBoardInitPreMemLib.inf
  }

  $(PLATFORM_PACKAGE)/PlatformInit/PlatformInitPei/PlatformInitPostMem.inf {
    <LibraryClasses>
    NULL|$(PROJECT)/Library/BoardInitLib/Pei/PeiMultiBoardInitPostMemLib.inf
  }
  $(PLATFORM_BOARD_PACKAGE)/OpenBoardPlatformInit/OpenBoardPlatformInitPei/OpenBoardPlatformInitPostMem.inf

  BoardModulePkg/FirmwareBootMediaInfo/FirmwareBootMediaInfoPei.inf

  $(PLATFORM_BOARD_PACKAGE)/BiosInfo/BiosInfo.inf
  $(PLATFORM_PACKAGE)/Services/StallServicePei/StallServicePei.inf

  $(PLATFORM_PACKAGE)/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPreMem.inf {
    <LibraryClasses>
      SiliconPolicyInitLib|$(PLATFORM_SI_PACKAGE)/Library/PeiPreMemSiliconPolicyInitLib/PeiPreMemSiliconPolicyInitLib.inf
      SiliconPolicyUpdateLib|$(PLATFORM_BOARD_PACKAGE)/Policy/Library/PeiSiliconPolicyUpdateLib/PeiSiliconPolicyUpdateLib.inf
  }

  $(PLATFORM_PACKAGE)/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPostMem.inf {
    <LibraryClasses>
      SiliconPolicyInitLib|$(PLATFORM_SI_PACKAGE)/Library/PeiPostMemSiliconPolicyInitLib/PeiPostMemSiliconPolicyInitLib.inf
      SiliconPolicyUpdateLib|$(PLATFORM_BOARD_PACKAGE)/Policy/Library/PeiSiliconPolicyUpdateLib/PeiSiliconPolicyUpdateLib.inf
  }

  IntelFsp2WrapperPkg/FspmWrapperPeim/FspmWrapperPeim.inf {
    <LibraryClasses>
        #
        # In FSP Dispatch mode below dummy library should be linked to bootloader PEIM
        # to build all DynamicEx PCDs that FSP consumes into bootloader PCD database.
        #
        NULL|$(PLATFORM_FSP_BIN_PACKAGE)/Library/FspPcdListLib/FspPcdListLibNull.inf
    }
  IntelFsp2WrapperPkg/FspsWrapperPeim/FspsWrapperPeim.inf {
    <LibraryClasses>
  }

  #
  # ADL FSP includes an older version of CpuMpPei, so the compatibility PEIM
  # is needed when using FSP Dispatch mode.
  #
!if gIntelFsp2WrapperTokenSpaceGuid.PcdFspModeSelection == 0
  $(PLATFORM_PACKAGE)/FspWrapper/MpInfo2HobPei/MpInfo2HobPei.inf
!endif

#
# Security
#
!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
  $(PLATFORM_PACKAGE)/Tcg/Tcg2PlatformPei/Tcg2PlatformPei.inf
!endif

#
# Features
#

 MdeModulePkg/Universal/ResetSystemPei/ResetSystemPei.inf {
   <LibraryClasses>
     ResetSystemLib|$(PLATFORM_SI_PACKAGE)/Pch/Library/BaseResetSystemLib/BaseResetSystemLib.inf
 }

[Components.X64]

#
# Common
#

!include $(PLATFORM_PACKAGE)/Include/Dsc/CoreDxeInclude.dsc

  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf {
    <LibraryClasses>
      # Use BaseDebugLibNull for save the binary size.
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  }

  MdeModulePkg/Universal/StatusCodeHandler/Smm/StatusCodeHandlerSmm.inf {
    <LibraryClasses>
      # Use BaseDebugLibNull for save the binary size.
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf

  }

  #
  # Generic EDKII Driver
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
    !if gPlatformModuleTokenSpaceGuid.PcdLzmaEnable == TRUE
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
    !endif
  }

  UefiCpuPkg/CpuDxe/CpuDxe.inf

  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf

  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Universal/Console/GraphicsOutputDxe/GraphicsOutputDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  BoardModulePkg/BoardBdsHookDxe/BoardBdsHookDxe.inf

#
#UEFI Shell
#
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf

    <PcdsFixedAtBuild>
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }

#
# Silicon
#
!include $(PLATFORM_SI_PACKAGE)/Product/Alderlake/SiPkgDxe.dsc
$(PLATFORM_SI_BIN_PACKAGE)/Microcode/MicrocodeUpdates.inf

#
# SMBIOS
#
!if gSiPkgTokenSpaceGuid.PcdSmbiosEnable == TRUE
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
!endif


#
# SmmAccess
#
  IntelSiliconPkg/Feature/SmmAccess/SmmAccessDxe/SmmAccess.inf

#
# Platform
#
  $(PLATFORM_PACKAGE)/PlatformInit/SiliconPolicyDxe/SiliconPolicyDxe.inf
  $(PLATFORM_PACKAGE)/PlatformInit/PlatformInitDxe/PlatformInitDxe.inf

  $(PLATFORM_PACKAGE)/Test/TestPointStubDxe/TestPointStubDxe.inf
  $(PLATFORM_PACKAGE)/Test/TestPointDumpApp/TestPointDumpApp.inf

!if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf
!endif #PcdBootToShellOnly

#
# OS Boot
#
!if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE
  $(PLATFORM_PACKAGE)/Acpi/AcpiTables/AcpiPlatform.inf
  $(PLATFORM_PACKAGE)/Acpi/MinDsdt/MinDsdt.inf
  $(PLATFORM_PACKAGE)/Acpi/AcpiSmm/AcpiSmm.inf {
    <LibraryClasses>
      NULL|$(PROJECT)/Library/BoardAcpiLib/SmmMultiBoardAcpiSupportLib.inf
  }

!if gSiPkgTokenSpaceGuid.PcdSmmVariableEnable == TRUE
  IntelSiliconPkg/Feature/Flash/SpiFvbService/SpiFvbServiceSmm.inf
!endif # gSiPkgTokenSpaceGuid.PcdSmmVariableEnable

!endif #PcdBootToShellOnly

#
# Security
#
  $(PLATFORM_PACKAGE)/Hsti/HstiIbvPlatformDxe/HstiIbvPlatformDxe.inf
!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
  $(PLATFORM_PACKAGE)/Tcg/Tcg2PlatformDxe/Tcg2PlatformDxe.inf
!endif

#
# Build Options
#
!include $(PLATFORM_SI_PACKAGE)/Product/Alderlake/SiPkgBuildOption.dsc
!include OpenBoardPkgBuildOption.dsc

