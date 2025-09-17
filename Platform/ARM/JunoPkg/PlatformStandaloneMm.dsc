## @file
# Standalone MM Platform.
#
# Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
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
  PLATFORM_GUID                  = 667ffb82-9128-11ef-8299-cfafe2cc85b5
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 0x0001001C
!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/ArmJuno
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/JunoPkg/PlatformStandaloneMm.fdf
  DEFINE DEBUG_MESSAGE           = TRUE

!include Platform/ARM/VExpressPkg/PlatformStandaloneMm.dsc.inc

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  # STMM for Variable runtime service.
!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  NorFlashDeviceLib|Platform/ARM/Library/P30NorFlashDeviceLib/P30NorFlashDeviceLib.inf
  NorFlashPlatformLib|Platform/ARM/JunoPkg/Library/NorFlashJunoLib/NorFlashJunoStMmLib.inf
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

  ## PL011 - Serial Terminal
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x7FF80000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultReceiveFifoDepth|0
  gArmPlatformTokenSpaceGuid.PL011UartClkInHz|7372800
  gArmPlatformTokenSpaceGuid.PL011UartInterrupt|115

  gEfiMdePkgTokenSpaceGuid.PcdMaximumGuidedExtractHandler|0x2

!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  #
  # NV Storage PCDs.
  # Use its base last 256KB block for NOR0 flash.
  # NOR0 base is 0x08000000 for and its size 64MB.
  # Therefore, 0x08000000 + 0x04000000 (64MB) - 0x40000 (256KB) = 0x0FFC0000.
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x0BFC0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x0BFD0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x0BFE0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00010000

  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800
!endif

  gEfiMdeModulePkgTokenSpaceGuid.PcdFfaLibConduitSmc|FALSE

  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x1C170000

  #
  # The BFV is not located in the Flash area but is loaded in the RAM
  # by TF-A instead, therefore no shadow copy is needed. So disable
  # shadow copy of boot firmware volume while loading StMM drivers.
  #
  gStandaloneMmPkgTokenSpaceGuid.PcdShadowBfv|FALSE

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
  GCC:*_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/Platform/ARM/JunoPkg/Include
  GCC:*_*_*_CC_FLAGS = -mstrict-align
