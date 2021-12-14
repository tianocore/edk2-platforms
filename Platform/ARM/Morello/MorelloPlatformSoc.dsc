## @file
#  Compoenent description file specific for Morello SoC Platform
#
#  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = morellosoc
  PLATFORM_GUID                  = 8AC37B62-713D-449D-876D-06AD1B8E67E5
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = NOOPT|DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/Morello/MorelloPlatformSoc.fdf
  BUILD_NUMBER                   = 1

  # Network definition
  DEFINE NETWORK_ISCSI_ENABLE    = FALSE

!include Platform/ARM/Morello/MorelloPlatform.dsc.inc
!include Platform/ARM/VExpressPkg/ArmVExpress.dsc.inc
!include DynamicTablesPkg/DynamicTables.dsc.inc
!include Platform/ARM/Morello/ConfigurationManager/ConfigurationManagerSoc.dsc.inc

# include common/basic libraries from MdePkg.
!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  # Platform Library
  ArmPlatformLib|Platform/ARM/Morello/Library/PlatformLib/PlatformLibSoc.inf

[Components.common]
  # Platform driver
  Platform/ARM/Morello/Drivers/PlatformDxe/PlatformDxeSoc.inf
