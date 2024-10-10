## @file
# Standalone MM Platform.
#
# Copyright (c) 2024-2025, Arm Limited. All rights reserved.<BR>
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
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/VExpressPkg/PlatformStandaloneMm.fdf
  DEFINE DEBUG_MESSAGE           = TRUE

!include Platform/ARM/VExpressPkg/PlatformStandaloneMm.dsc.inc

  # To allow firmware update using capsule update framwork.
  DEFINE ENABLE_FIRMWARE_UPDATE                  = FALSE

!if $(ENABLE_FIRMWARE_UPDATE) == TRUE && $(ENABLE_UEFI_SECURE_VARIABLE) == FALSE
  !error "ENABLE_UEFI_SECURE_VARIABLE should be on when ENABLE_FIRMWARE_UPDATE is on."
!endif

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

[LibraryClasses]
  # STMM for Variable runtime service.
!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE || $(ENABLE_FIRMWARE_UPDATE) == TRUE || $(ENABLE_TPM) == TRUE
  NorFlashDeviceLib|Platform/ARM/Library/P30NorFlashDeviceLib/P30NorFlashDeviceLib.inf
  NorFlashPlatformLib|Platform/ARM/VExpressPkg/Library/NorFlashArmVExpressLib/NorFlashStMmLib.inf
!endif

!if $(ENABLE_FIRMWARE_UPDATE) == TRUE
  FwsPlatformLib|Platform/ARM/Library/FwsGptSystemFipLib/FwsGptSystemFipLib.inf
!endif

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x21
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f
!endif

  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8000008F
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

  gEfiMdeModulePkgTokenSpaceGuid.PcdFfaLibConduitSmc|FALSE

!if $(ENABLE_TPM) == TRUE
  #
  # fTPM uses a significant amount of stack memory when handling TPM commands,
  # for example during cryptographic operations.
  # Therefore, the stack size should be increased when fTPM is enabled.
  #
  gArmTokenSpaceGuid.PcdStMmStackSize|0x4000

  #
  # Normal pseudo crbs which locality from 0 to 3 are allocated
  # at the start of System Memory.
  # These regions are reserved by TF-A.
  #
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmBaseAddress|0xfef10000
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmMaxAddress|0xfef13fff
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmCrbRegionSize|0x4000

  #
  # Secure pseudo crb is allocated at the end of StandaloneMm's memory area
  # as much as PcdTpmSecureCrbSize which default is 0x1000.
  # This region is reserved by TF-A.
  #
  gPlatformArmTokenSpaceGuid.PcdTpmSecureCrbBase|0xffdfe000

  #
  # The second last 256KB block is used for TPM storage in norflash1.
  # The end of norflash1 device address is 0x10000000.
  # Therefore 0x10000000 - 0x400000 (512KB) = 0xFF800000
  #
  gPlatformArmTokenSpaceGuid.PcdTpmNvMemoryBase|0x0FF80000
!endif

  #
  # The BFV is not located in the Flash area but is loaded in the RAM
  # by TF-A instead, therefore no shadow copy is needed. So disable
  # shadow copy of boot firmware volume while loading StMM drivers.
  #
  gStandaloneMmPkgTokenSpaceGuid.PcdShadowBfv|FALSE

!if $(ENABLE_FIRMWARE_UPDATE) == TRUE
  # Firmware storage configuration
  # Firmware storage layout is based on the underlying TF-A implementation.
  # For the Base FVP:
  #
  #  +----------------------+
  #  |      GPT-HEADER      |
  #  +----------------------+
  #  |    FIP_A (bank0)     |  -> bank 0 ---
  #  +----------------------+              | --> 2 banks. and each bank only
  #  |    FIP_B (bank1)     |  -> bank 1 ---     has one image (fip).
  #  +----------------------+
  #  |    FWU-Metadata      |
  #  +----------------------+
  #  |  Bkup-FWU-Metadata   |
  #  +----------------------+
  gPlatformArmTokenSpaceGuid.PcdFwuNumberOfBanks|2
  gPlatformArmTokenSpaceGuid.PcdFwuImagesPerBank|1
  gPlatformArmTokenSpaceGuid.PcdFlashNvStorageFwuBase|0x08000000
  gPlatformArmTokenSpaceGuid.PcdFlashNvStorageFwuSize|0x04000000

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
!if $(ENABLE_FIRMWARE_UPDATE) == TRUE
  Platform/ARM/Drivers/FwuSmm/FwuSmm.inf {
    <PcdsFixedAtBuild>
      # Should be same to FmpSystemFipImage.
      gPlatformArmTokenSpaceGuid.PcdSystemFirmwareFmpLowestSupportedVersion|0x00000000
      gPlatformArmTokenSpaceGuid.PcdSystemFirmwareFmpVersion|0x00000000
      gPlatformArmTokenSpaceGuid.PcdSystemFirmwareFmpVersionString|"000.000.000.000"
  }
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
!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  GCC:*_*_*_CC_FLAGS = -DENABLE_UEFI_SECURE_VARIABLE
!endif
!if $(ENABLE_FIRMWARE_UPDATE) == TRUE
  GCC:*_*_*_CC_FLAGS = -DENABLE_FIRMWARE_UPDATE
!endif
