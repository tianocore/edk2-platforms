## @file
# Standalone MM Platform.
#
# Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = StandaloneMm
  PLATFORM_GUID                  = 9A4BBA60-B4F9-47C7-9258-3BD77CAE9322
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 0x0001001C
!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/ArmVExpress-FVP-AArch64
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64|ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/VExpressPkg/PlatformStandaloneMm.fdf
  DEFINE DEBUG_MESSAGE           = TRUE

  # To allow the use of ueif secure variable feature, set this to TRUE.
  DEFINE ENABLE_UEFI_SECURE_VARIABLE = FALSE

  # LzmaF86
  DEFINE COMPRESSION_TOOL_GUID   = D42AE6BD-1352-4bfb-909A-CA72A6EAE889

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
  FvLib|StandaloneMmPkg/Library/FvLib/FvLib.inf
  HobLib|StandaloneMmPkg/Library/StandaloneMmHobLib/StandaloneMmHobLib.inf
  HobPrintLib|MdeModulePkg/Library/HobPrintLib/HobPrintLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MemLib|StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf
  MemoryAllocationLib|StandaloneMmPkg/Library/StandaloneMmCoreMemoryAllocationLib/StandaloneMmCoreMemoryAllocationLib.inf
  MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  StandaloneMmCoreEntryPoint|StandaloneMmPkg/Library/StandaloneMmCoreEntryPoint/StandaloneMmCoreEntryPoint.inf
  StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
  VariableMmDependency|StandaloneMmPkg/Library/VariableMmDependency/VariableMmDependency.inf

  # ARM PL011 UART Driver
  PL011UartClockLib|ArmPlatformPkg/Library/PL011UartClockLib/PL011UartClockLib.inf
  PL011UartLib|ArmPlatformPkg/Library/PL011UartLib/PL011UartLib.inf
  SerialPortLib|ArmPlatformPkg/Library/PL011SerialPortLib/PL011SerialPortLib.inf

[LibraryClasses.AARCH64, LibraryClasses.ARM]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  StandaloneMmMmuLib|ArmPkg/Library/StandaloneMmMmuLib/ArmMmuStandaloneMmLib.inf
  ArmSvcLib|ArmPkg/Library/ArmSvcLib/ArmSvcLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  PeCoffExtraActionLib|StandaloneMmPkg/Library/StandaloneMmPeCoffExtraActionLib/StandaloneMmPeCoffExtraActionLib.inf

[LibraryClasses.common.MM_CORE_STANDALONE]
  HobLib|StandaloneMmPkg/Library/StandaloneMmCoreHobLib/StandaloneMmCoreHobLib.inf

[LibraryClasses.common.MM_STANDALONE]
  MemoryAllocationLib|StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib.inf

  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  RngLib|MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf
  PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

  # STMM for Variable runtime service.
!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  NorFlashDeviceLib|Platform/ARM/Library/P30NorFlashDeviceLib/P30NorFlashDeviceLib.inf
  NorFlashPlatformLib|Platform/ARM/VExpressPkg/Library/NorFlashArmVExpressLib/NorFlashStMmLib.inf
  # NOR flash identification support
  NorFlashInfoLib|EmbeddedPkg/Library/NorFlashInfoLib/NorFlashInfoLib.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
!endif

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8000008F
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xff
  gEfiMdePkgTokenSpaceGuid.PcdDebugClearMemoryValue|0xAF

  ## PL011 - Serial Terminal.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x1c090000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
  gEfiMdePkgTokenSpaceGuid.PcdMaximumGuidedExtractHandler|0x2

  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800

  #
  # NV Storage PCDs.
  # Use its base last 256KB block for NOR1 flash.
  # NOR1 base is 0x0C000000 for and its size 64MB.
  # Therefore, 0x0C000000 + 0x04000000 (64MB) - 0x40000 (256KB) = 0x0FFC0000.
  #
!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x0FFC0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x0FFD0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x0FFE0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00010000
!endif

###################################################################################################
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
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################
[Components.common]
  #
  # MM Core
  #
  StandaloneMmPkg/Core/StandaloneMmCore.inf
  StandaloneMmPkg/Library/FvLib/FvLib.inf
  StandaloneMmPkg/Library/StandaloneMmCoreEntryPoint/StandaloneMmCoreEntryPoint.inf
  StandaloneMmPkg/Library/StandaloneMmCoreHobLib/StandaloneMmCoreHobLib.inf
  StandaloneMmPkg/Library/StandaloneMmCoreMemoryAllocationLib/StandaloneMmCoreMemoryAllocationLib.inf
  StandaloneMmPkg/Library/StandaloneMmHobLib/StandaloneMmHobLib.inf
  StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf
  StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib.inf
  StandaloneMmPkg/Library/VariableMmDependency/VariableMmDependency.inf

[Components.AARCH64, Components.ARM]
  StandaloneMmPkg/Drivers/StandaloneMmCpu/StandaloneMmCpu.inf
  StandaloneMmPkg/Library/StandaloneMmPeCoffExtraActionLib/StandaloneMmPeCoffExtraActionLib.inf

!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  Platform/ARM/Drivers/NorFlashDxe/NorFlashStandaloneMm.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableStandaloneMm.inf {
    <LibraryClasses>
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLibStandaloneMm.inf
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
      VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
      VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  }
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteStandaloneMm.inf
!endif

###################################################################################################
#
# BuildOptions Section - Define the module specific tool chain flags that should be used as
#                        the default flags for a module. These flags are appended to any
#                        standard flags that are defined by the build process. They can be
#                        applied for any modules or only those modules with the specific
#                        module style (EDK or EDKII) specified in [Components] section.
#
###################################################################################################
[BuildOptions.AARCH64]
  GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000 -march=armv8-a+nofp -mstrict-align
  GCC:*_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/Platform/ARM/VExpressPkg/Include/Platform/RTSM
  GCC:*_*_*_CC_FLAGS = -mstrict-align

[BuildOptions.ARM]
  GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000 -march=armv7-a
  GCC:*_*_*_CC_FLAGS = -fno-stack-protector
