## @file
#  The main build description file for the UpXtreme board.
#
#  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DEFINE      PLATFORM_PACKAGE          = MinPlatformPkg
  DEFINE      PLATFORM_SI_PACKAGE       = CoffeelakeSiliconPkg
  DEFINE      PLATFORM_SI_BIN_PACKAGE   = CoffeelakeSiliconBinPkg
  DEFINE      PLATFORM_FSP_BIN_PACKAGE  = CoffeeLakeFspBinPkg
  DEFINE      PLATFORM_BOARD_PACKAGE    = WhiskeylakeOpenBoardPkg
  DEFINE      BOARD                     = UpXtreme
  DEFINE      PROJECT                   = $(PLATFORM_BOARD_PACKAGE)/$(BOARD)
  DEFINE      PEI_ARCH                  = IA32
  DEFINE      DXE_ARCH                  = X64
  DEFINE      TOP_MEMORY_ADDRESS        = 0x0
  
  
  
  # Defined for Capsule Update, we generated this GUID
  DEFINE FMP_WHISKEY_LAKE_SYSTEM =  214a15d8-7c3b-4a1c-b63b-dee0251beb38

  #
  # Copied from MinnowMaxConfig.dsc
  # Capsule Pubic Certificate.  Default is EDK_TEST.  Options are:
  #   SAMPLE_DEVELOPMENT                    - Only signtool SAMPLE_DEVELOPMENT
  #   SAMPLE_DEVELOPMENT_SAMPLE_PRODUCTION  - Both signtool SAMPLE_DEVELOPMENT and SAMPLE_PRODUCTION
  #   EDKII_TEST                            - Only openssl EDK II test certificate
  #   NEW_ROOT                              - Only openssl new VLV2 certificate
  #
  DEFINE CAPSULE_PKCS7_CERT = EDKII_TEST

  #
  # Used to enable/disable capsule update features.  The default is FALSE for disabled.
  # Add -D CAPSULE_ENABLE to the build command line to enable capsule update features.
  # The build process generates a capsule update image along with the UEFI application 
  # CapsuleApp.efi.  These 2 files must be transferred to storage media to in order for 
  # a user to boot to UEFI Shell and use CapsuleApp.efi to submit the signed capsule.
  # Once the system is rebooted, the signed capsule is authenticated and the firmware is
  # update with the new system firmware version.
  #
  DEFINE CAPSULE_ENABLE = TRUE

  #
  # Default value for OpenBoardPkg.fdf use
  #
  DEFINE BIOS_SIZE_OPTION = SIZE_80

  PLATFORM_NAME                       = $(PLATFORM_PACKAGE)
  PLATFORM_GUID                       = A12B2802-BF37-4886-A307-C060F7929F8F
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  OUTPUT_DIRECTORY                    = Build/$(PROJECT)
  SUPPORTED_ARCHITECTURES             = IA32|X64
  BUILD_TARGETS                       = DEBUG|RELEASE
  SKUID_IDENTIFIER                    = ALL

  FLASH_DEFINITION                    = $(PROJECT)/OpenBoardPkg.fdf
  FIX_LOAD_TOP_MEMORY_ADDRESS         = 0x0

  #
  # Include PCD configuration for this board.
  #
  !include AdvancedFeaturePkg/Include/AdvancedFeaturesPcd.dsc

  !include OpenBoardPkgPcd.dsc
  !include AdvancedFeaturePkg/Include/AdvancedFeatures.dsc

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this board.
#
################################################################################
[SkuIds]
  0|DEFAULT              # 0|DEFAULT is reserved and always required.
  0x10|UpXtreme

################################################################################
#
# Includes section - other DSC file contents included for this board build.
#
################################################################################

#######################################
# Library Includes
#######################################
!include $(PLATFORM_PACKAGE)/Include/Dsc/CoreCommonLib.dsc
!include $(PLATFORM_PACKAGE)/Include/Dsc/CorePeiLib.dsc
!include $(PLATFORM_PACKAGE)/Include/Dsc/CoreDxeLib.dsc
!include $(PLATFORM_SI_PACKAGE)/SiPkgCommonLib.dsc
!include $(PLATFORM_SI_PACKAGE)/SiPkgPeiLib.dsc
!include $(PLATFORM_SI_PACKAGE)/SiPkgDxeLib.dsc

#######################################
# Component Includes
#######################################
# @todo: Change below line to [Components.$(PEI_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed
[Components.IA32]
!include $(PLATFORM_PACKAGE)/Include/Dsc/CorePeiInclude.dsc
!include $(PLATFORM_SI_PACKAGE)/SiPkgPei.dsc

# @todo: Change below line to [Components.$(DXE_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed
[Components.X64]
!include $(PLATFORM_PACKAGE)/Include/Dsc/CoreDxeInclude.dsc
!include $(PLATFORM_SI_PACKAGE)/SiPkgDxe.dsc


!if $(CAPSULE_ENABLE)
  MdeModulePkg/Universal/EsrtFmpDxe/EsrtFmpDxe.inf
  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf
!endif

!if $(CAPSULE_ENABLE)
  !include WhiskeylakeOpenBoardPkg/UpXtreme/FmpWhiskeyLakeSystem.dsc
!endif

#######################################
# Build Option Includes
#######################################
!include $(PLATFORM_SI_PACKAGE)/SiPkgBuildOption.dsc
!include OpenBoardPkgBuildOption.dsc

################################################################################
#
# Library Class section - list of all Library Classes needed by this board.
#
################################################################################

[LibraryClasses]
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!if $(CAPSULE_ENABLE)
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf
!else
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
!endif
  EdkiiSystemCapsuleLib|SignedCapsulePkg/Library/EdkiiSystemCapsuleLib/EdkiiSystemCapsuleLib.inf
  FmpAuthenticationLib|MdeModulePkg/Library/FmpAuthenticationLibNull/FmpAuthenticationLibNull.inf
  IniParsingLib|SignedCapsulePkg/Library/IniParsingLib/IniParsingLib.inf
  #PlatformFlashAccessLib|WhiskeylakeOpenBoardPkg/Features/Capsule/Library/PlatformFlashAccessLib/PlatformFlashAccessLibDxe.inf
  PlatformFlashAccessLib|SignedCapsulePkg\Library\PlatformFlashAccessLibNull\PlatformFlashAccessLibNull.inf

[LibraryClasses.common]
  #######################################
  # Edk2 Packages
  #######################################
  FspWrapperApiLib|IntelFsp2WrapperPkg/Library/BaseFspWrapperApiLib/BaseFspWrapperApiLib.inf
  FspWrapperApiTestLib|IntelFsp2WrapperPkg/Library/PeiFspWrapperApiTestLib/PeiFspWrapperApiTestLib.inf

  #######################################
  # Silicon Initialization Package
  #######################################
  ConfigBlockLib|IntelSiliconPkg/Library/BaseConfigBlockLib/BaseConfigBlockLib.inf
  MmPciLib|$(PLATFORM_SI_PACKAGE)/Library/PeiDxeSmmMmPciLib/PeiDxeSmmMmPciLib.inf
  PchHsioLib|$(PLATFORM_SI_PACKAGE)/Pch/Library/PeiDxeSmmPchHsioLib/PeiDxeSmmPchHsioLib.inf
  PchPmcLib|$(PLATFORM_SI_PACKAGE)/Pch/Library/PeiDxeSmmPchPmcLib/PeiDxeSmmPchPmcLib.inf

  #####################################
  # Platform Package
  #####################################
  BoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/BoardInitLibNull/BoardInitLibNull.inf
  FspWrapperHobProcessLib|$(PLATFORM_PACKAGE)/FspWrapper/Library/PeiFspWrapperHobProcessLib/PeiFspWrapperHobProcessLib.inf
  FspWrapperPlatformLib|$(PLATFORM_PACKAGE)/FspWrapper/Library/PeiFspWrapperPlatformLib/PeiFspWrapperPlatformLib.inf
  PciHostBridgeLib|$(PLATFORM_PACKAGE)/Pci/Library/PciHostBridgeLibSimple/PciHostBridgeLibSimple.inf
  PciSegmentInfoLib|$(PLATFORM_PACKAGE)/Pci/Library/PciSegmentInfoLibSimple/PciSegmentInfoLibSimple.inf
  PeiLib|$(PLATFORM_PACKAGE)/Library/PeiLib/PeiLib.inf
  PlatformBootManagerLib|$(PLATFORM_PACKAGE)/Bds/Library/DxePlatformBootManagerLib/DxePlatformBootManagerLib.inf
  ReportFvLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/PeiReportFvLib/PeiReportFvLib.inf
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf

  #######################################
  # Board Package
  #######################################
  GpioExpanderLib|$(PLATFORM_BOARD_PACKAGE)/Library/BaseGpioExpanderLib/BaseGpioExpanderLib.inf
  HdaVerbTableLib|$(PLATFORM_BOARD_PACKAGE)/Library/PeiHdaVerbTableLib/PeiHdaVerbTableLib.inf
  I2cAccessLib|$(PLATFORM_BOARD_PACKAGE)/Library/PeiI2cAccessLib/PeiI2cAccessLib.inf
  PlatformSecLib|$(PROJECT)/FspWrapper/Library/SecFspWrapperPlatformSecLib/SecFspWrapperPlatformSecLib.inf
  TimerLib|$(PLATFORM_BOARD_PACKAGE)/Library/AcpiTimerLib/BaseAcpiTimerLib.inf
  # Thunderbolt
!if gWhiskeylakeOpenBoardPkgTokenSpaceGuid.PcdTbtEnable == TRUE
  TbtCommonLib|$(PLATFORM_BOARD_PACKAGE)/Features/Tbt/Library/PeiDxeSmmTbtCommonLib/TbtCommonLib.inf
!endif

  #######################################
  # Board-specific
  #######################################
  PlatformHookLib|$(PROJECT)/Library/BasePlatformHookLib/BasePlatformHookLib.inf

[LibraryClasses.IA32.SEC]
  #######################################
  # Platform Package
  #######################################
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/SecTestPointCheckLib.inf
  SecBoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/SecBoardInitLibNull/SecBoardInitLibNull.inf

  #######################################
  # Board Package
  #######################################
  SiliconPolicyInitLib|$(PLATFORM_BOARD_PACKAGE)/FspWrapper/Library/PeiFspPolicyInitLib/PeiFspPolicyInitLib.inf
  SiliconPolicyUpdateLib|$(PROJECT)/FspWrapper/Library/PeiSiliconPolicyUpdateLibFsp/PeiSiliconPolicyUpdateLibFsp.inf
  TimerLib|$(PLATFORM_BOARD_PACKAGE)/Library/AcpiTimerLib/BaseAcpiTimerLib.inf

[LibraryClasses.common.PEIM]
  #######################################
  # Silicon Initialization Package
  #######################################
  SiliconInitLib|$(PLATFORM_SI_PACKAGE)/Library/PeiSiliconInitLib/PeiSiliconInitLib.inf

  #######################################
  # Platform Package
  #######################################
  BoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/PeiMultiBoardInitSupportLib.inf
  FspWrapperPlatformLib|$(PLATFORM_PACKAGE)/FspWrapper/Library/PeiFspWrapperPlatformLib/PeiFspWrapperPlatformLib.inf
  MultiBoardInitSupportLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/PeiMultiBoardInitSupportLib.inf
  TestPointLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointLib/PeiTestPointLib.inf
!if $(TARGET) == DEBUG
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/PeiTestPointCheckLib.inf
!endif
  SetCacheMtrrLib|$(PLATFORM_PACKAGE)/Library/SetCacheMtrrLib/SetCacheMtrrLibNull.inf
  ReportCpuHobLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/ReportCpuHobLib/ReportCpuHobLib.inf

  #######################################
  # Board Package
  #######################################
  # Thunderbolt
!if gWhiskeylakeOpenBoardPkgTokenSpaceGuid.PcdTbtEnable == TRUE
  PeiDTbtInitLib|$(PLATFORM_BOARD_PACKAGE)/Features/Tbt/Library/Private/PeiDTbtInitLib/PeiDTbtInitLib.inf
  PeiTbtPolicyLib|$(PLATFORM_BOARD_PACKAGE)/Features/Tbt/Library/PeiTbtPolicyLib/PeiTbtPolicyLib.inf
!endif
  PeiPolicyInitLib|$(PLATFORM_BOARD_PACKAGE)/Policy/Library/PeiPolicyInitLib/PeiPolicyInitLib.inf
  PeiPolicyUpdateLib|$(PLATFORM_BOARD_PACKAGE)/Policy/Library/PeiPolicyUpdateLib/PeiPolicyUpdateLib.inf
  SiliconPolicyInitLib|$(PLATFORM_BOARD_PACKAGE)/FspWrapper/Library/PeiFspPolicyInitLib/PeiFspPolicyInitLib.inf
  SiliconPolicyUpdateLib|$(PROJECT)/FspWrapper/Library/PeiSiliconPolicyUpdateLibFsp/PeiSiliconPolicyUpdateLibFsp.inf
  TimerLib|$(PLATFORM_BOARD_PACKAGE)/Library/AcpiTimerLib/BaseAcpiTimerLib.inf

  #######################################
  # Board-specific
  #######################################
  PeiPlatformHookLib|$(PROJECT)/Library/PeiPlatformHookLib/PeiPlatformHookLib.inf
  PeiPolicyBoardConfigLib|$(PROJECT)/Library/PeiPolicyBoardConfigLib/PeiPolicyBoardConfigLib.inf

!if $(TARGET) == DEBUG
  GpioCheckConflictLib|$(PROJECT)/Library/BaseGpioCheckConflictLib/BaseGpioCheckConflictLib.inf
!else
  GpioCheckConflictLib|$(PROJECT)/Library/BaseGpioCheckConflictLibNull/BaseGpioCheckConflictLibNull.inf
!endif

[LibraryClasses.common.DXE_DRIVER]
  #######################################
  # Edk2 Packages
  #######################################
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

  #######################################
  # Platform Package
  #######################################
  BoardAcpiTableLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/DxeMultiBoardAcpiSupportLib.inf
  BoardInitLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/DxeMultiBoardInitSupportLib.inf
  FspWrapperPlatformLib|$(PLATFORM_PACKAGE)/FspWrapper/Library/DxeFspWrapperPlatformLib/DxeFspWrapperPlatformLib.inf
  MultiBoardAcpiSupportLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/DxeMultiBoardAcpiSupportLib.inf
  MultiBoardInitSupportLib|$(PLATFORM_PACKAGE)/PlatformInit/Library/MultiBoardInitSupportLib/DxeMultiBoardInitSupportLib.inf
  TestPointLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointLib/DxeTestPointLib.inf

!if $(TARGET) == DEBUG
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/DxeTestPointCheckLib.inf
!endif

  #######################################
  # Board Package
  #######################################
  DxePolicyUpdateLib|$(PLATFORM_BOARD_PACKAGE)/Policy/Library/DxePolicyUpdateLib/DxePolicyUpdateLib.inf
  DxeTbtPolicyLib|$(PLATFORM_BOARD_PACKAGE)/Features/Tbt/Library/DxeTbtPolicyLib/DxeTbtPolicyLib.inf
  BoardBdsHookLib|BoardModulePkg/Library/BoardBdsHookLib/BoardBdsHookLib.inf
  BoardBootManagerLib|BoardModulePkg/Library/BoardBootManagerLib/BoardBootManagerLib.inf

  #######################################
  # Board-specific
  #######################################
  DxePolicyBoardConfigLib|$(PROJECT)/Library/DxePolicyBoardConfigLib/DxePolicyBoardConfigLib.inf


[LibraryClasses.common.DXE_RUNTIME_DRIVER]
!if $(CAPSULE_ENABLE)
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeRuntimeCapsuleLib.inf
!endif



[LibraryClasses.X64.DXE_RUNTIME_DRIVER]

  #######################################
  # Edk2 Packages
  #######################################
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

  #######################################
  # Silicon Initialization Package
  #######################################
  ResetSystemLib|$(PLATFORM_SI_PACKAGE)/Pch/Library/DxeRuntimeResetSystemLib/DxeRuntimeResetSystemLib.inf

[LibraryClasses.X64.DXE_SMM_DRIVER]
  #######################################
  # Edk2 Packages
  #######################################
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

  #######################################
  # Silicon Initialization Package
  #######################################
  SpiFlashCommonLib|$(PLATFORM_SI_PACKAGE)/Pch/Library/SmmSpiFlashCommonLib/SmmSpiFlashCommonLib.inf

  #######################################
  # Platform Package
  #######################################
  BoardAcpiEnableLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/SmmMultiBoardAcpiSupportLib.inf
  MultiBoardAcpiSupportLib|$(PLATFORM_PACKAGE)/Acpi/Library/MultiBoardAcpiSupportLib/SmmMultiBoardAcpiSupportLib.inf
  TestPointLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointLib/SmmTestPointLib.inf
!if $(TARGET) == DEBUG
  TestPointCheckLib|$(PLATFORM_PACKAGE)/Test/Library/TestPointCheckLib/SmmTestPointCheckLib.inf
!endif

#######################################
# PEI Components
#######################################
# @todo: Change below line to [Components.$(PEI_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed
[Components.IA32]
  #######################################
  # Edk2 Packages
  #######################################
  UefiCpuPkg/SecCore/SecCore.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  }

  #
  # In FSP API mode the policy has to be installed before FSP Wrapper updating UPD.
  # Add policy as dependency for FSP Wrapper
  #
  IntelFsp2WrapperPkg/FspmWrapperPeim/FspmWrapperPeim.inf
  IntelFsp2WrapperPkg/FspsWrapperPeim/FspsWrapperPeim.inf

  #######################################
  # Silicon Initialization Package
  #######################################
  IntelSiliconPkg/Feature/VTd/IntelVTdPmrPei/IntelVTdPmrPei.inf
  IntelSiliconPkg/Feature/VTd/PlatformVTdInfoSamplePei/PlatformVTdInfoSamplePei.inf

  #######################################
  # Platform Package
  #######################################
  $(PLATFORM_PACKAGE)/PlatformInit/ReportFv/ReportFvPei.inf
  $(PLATFORM_PACKAGE)/PlatformInit/PlatformInitPei/PlatformInitPreMem.inf {
    <LibraryClasses>
      !if gWhiskeylakeOpenBoardPkgTokenSpaceGuid.PcdMultiBoardSupport == FALSE
        BoardInitLib|$(PROJECT)/Library/BoardInitLib/PeiBoardInitPreMemLib.inf
      !else
        NULL|$(PROJECT)/Library/BoardInitLib/PeiMultiBoardInitPreMemLib.inf
      !endif
      NULL|$(PROJECT)/Library/BaseFuncLib/BaseFuncLib.inf
  }

  $(PLATFORM_PACKAGE)/PlatformInit/PlatformInitPei/PlatformInitPostMem.inf {
    <LibraryClasses>
      !if gWhiskeylakeOpenBoardPkgTokenSpaceGuid.PcdMultiBoardSupport == FALSE
        BoardInitLib|$(PROJECT)/Library/BoardInitLib/PeiBoardInitPostMemLib.inf
      !else
        NULL|$(PROJECT)/Library/BoardInitLib/PeiMultiBoardInitPostMemLib.inf
      !endif
  }

  $(PLATFORM_PACKAGE)/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPreMem.inf
  $(PLATFORM_PACKAGE)/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPostMem.inf

!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
  $(PLATFORM_PACKAGE)/Tcg/Tcg2PlatformPei/Tcg2PlatformPei.inf
!endif


  #######################################
  # Board Package
  #######################################
  # Thunderbolt
!if gWhiskeylakeOpenBoardPkgTokenSpaceGuid.PcdTbtEnable == TRUE
  $(PLATFORM_BOARD_PACKAGE)/Features/Tbt/TbtInit/Pei/PeiTbtInit.inf
!endif
  $(PLATFORM_BOARD_PACKAGE)/BiosInfo/BiosInfo.inf

#######################################
# DXE Components
#######################################
# @todo: Change below line to [Components.$(DXE_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed


!if $(CAPSULE_ENABLE) == TRUE
  MdeModulePkg/Universal/CapsulePei/CapsulePei.inf
!endif

[Components.X64]

!if $(CAPSULE_ENABLE) == TRUE
  MdeModulePkg/Universal/CapsulePei/CapsuleX64.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
      MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
      HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
      CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf

  }
!endif

!if $(CAPSULE_ENABLE) || $(MICOCODE_CAPSULE_ENABLE)
  MdeModulePkg/Universal/EsrtFmpDxe/EsrtFmpDxe.inf
  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf
!endif

!if $(CAPSULE_ENABLE)
  !include FmpWhiskeyLakeSystem.dsc
!endif

!if $(MICOCODE_CAPSULE_ENABLE)
  IntelSiliconPkg/Feature/Capsule/MicrocodeUpdateDxe/MicrocodeUpdateDxe.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  }
!endif



  #######################################
  # Edk2 Packages
  #######################################
  IntelFsp2WrapperPkg/FspWrapperNotifyDxe/FspWrapperNotifyDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Universal/Console/GraphicsOutputDxe/GraphicsOutputDxe.inf
  UefiCpuPkg/CpuDxe/CpuDxe.inf

  #
  # eMMC/SD Card
  #
  MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.inf
  MdeModulePkg/Bus/Sd/EmmcDxe/EmmcDxe.inf
  MdeModulePkg/Bus/Sd/SdDxe/SdDxe.inf

  ShellPkg/Application/Shell/Shell.inf {
   <PcdsFixedAtBuild>
     gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
   <LibraryClasses>
     NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
     NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
     NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
     NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
     NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
     NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
     NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
     NULL|ShellPkg/Library/UefiShellNetwork2CommandsLib/UefiShellNetwork2CommandsLib.inf
     ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
     HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
     BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
     ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
     ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  }

!if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80080046
    <LibraryClasses>
      !if $(TARGET) == DEBUG
        DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      !endif
  }
!endif

  #######################################
  # Silicon Initialization Package
  #######################################
  IntelSiliconPkg/Feature/VTd/IntelVTdDxe/IntelVTdDxe.inf
  $(PLATFORM_SI_PACKAGE)/SystemAgent/SaInit/Dxe/SaInitDxe.inf
  $(PLATFORM_SI_BIN_PACKAGE)/Microcode/MicrocodeUpdates.inf

  #######################################
  # Platform Package
  #######################################
  $(PLATFORM_PACKAGE)/FspWrapper/SaveMemoryConfig/SaveMemoryConfig.inf
  $(PLATFORM_PACKAGE)/Hsti/HstiIbvPlatformDxe/HstiIbvPlatformDxe.inf
  $(PLATFORM_PACKAGE)/PlatformInit/PlatformInitDxe/PlatformInitDxe.inf
  $(PLATFORM_PACKAGE)/PlatformInit/SiliconPolicyDxe/SiliconPolicyDxe.inf {
    <LibraryClasses>
      SiliconPolicyInitLib|MinPlatformPkg/PlatformInit/Library/SiliconPolicyInitLibNull/SiliconPolicyInitLibNull.inf
      SiliconPolicyUpdateLib|MinPlatformPkg/PlatformInit/Library/SiliconPolicyUpdateLibNull/SiliconPolicyUpdateLibNull.inf
  }
  $(PLATFORM_PACKAGE)/Test/TestPointDumpApp/TestPointDumpApp.inf
  $(PLATFORM_PACKAGE)/Test/TestPointStubDxe/TestPointStubDxe.inf

!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
  $(PLATFORM_PACKAGE)/Tcg/Tcg2PlatformDxe/Tcg2PlatformDxe.inf
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE

  $(PLATFORM_PACKAGE)/Flash/SpiFvbService/SpiFvbServiceSmm.inf
  $(PLATFORM_PACKAGE)/PlatformInit/PlatformInitSmm/PlatformInitSmm.inf

  $(PLATFORM_PACKAGE)/Acpi/AcpiSmm/AcpiSmm.inf {
    <LibraryClasses>
      !if gWhiskeylakeOpenBoardPkgTokenSpaceGuid.PcdMultiBoardSupport == FALSE
        BoardAcpiEnableLib|$(PROJECT)/Library/BoardAcpiLib/SmmBoardAcpiEnableLib.inf
      !else
        NULL|$(PROJECT)/Library/BoardAcpiLib/SmmMultiBoardAcpiSupportLib.inf
      !endif
  }

  $(PLATFORM_PACKAGE)/Acpi/AcpiTables/AcpiPlatform.inf

!endif

  #######################################
  # Board Package
  #######################################
  $(PLATFORM_BOARD_PACKAGE)/Policy/PolicyInitDxe/PolicyInitDxe.inf{
    <LibraryClasses>
      NULL|$(PROJECT)/Library/BaseFuncLib/BaseFuncLib.inf
  }

  # Thunderbolt
!if gWhiskeylakeOpenBoardPkgTokenSpaceGuid.PcdTbtEnable == TRUE
  $(PLATFORM_BOARD_PACKAGE)/Features/Tbt/TbtInit/Smm/TbtSmm.inf
  $(PLATFORM_BOARD_PACKAGE)/Features/Tbt/TbtInit/Dxe/TbtDxe.inf
  $(PLATFORM_BOARD_PACKAGE)/Features/PciHotPlug/PciHotPlug.inf
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE
  $(PLATFORM_BOARD_PACKAGE)/Acpi/BoardAcpiDxe/BoardAcpiDxe.inf
!endif
  BoardModulePkg/LegacySioDxe/LegacySioDxe.inf
  BoardModulePkg/BoardBdsHookDxe/BoardBdsHookDxe.inf


#######################################
# PCD Section
#######################################

[PcdsDynamicExDefault.common.DEFAULT]
!if $(CAPSULE_ENABLE)
  #gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareImageDescriptor|{0x0}|VOID*|0x100 
  gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{GUID("$(FMP_WHISKEY_LAKE_SYSTEM)")}|VOID*|0x10
  #gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareFileGuid|{0x31, 0x1c, 0x96, 0x20, 0xdc, 0x66, 0xa5, 0x48, 0x89, 0x1c, 0x25, 0x43, 0x8b, 0xde, 0x14, 0x30}
!endif

