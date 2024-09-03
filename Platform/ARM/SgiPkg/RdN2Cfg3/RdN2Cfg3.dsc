#
#  Copyright (c) 2024, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = RdN2Cfg3
  PLATFORM_GUID                  = b890ba7d-a256-4820-9d3a-655acbb737c9
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = NOOPT|DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/SgiPkg/SgiPlatform.fdf
  BOARD_DXE_FV_COMPONENTS        = Platform/ARM/SgiPkg/RdN2Cfg3/RdN2Cfg3.fdf.inc
  BUILD_NUMBER                   = 1

# include common/basic libraries from MdePkg.
!include MdePkg/MdeLibs.dsc.inc

# include common definitions from SgiPlatform.dsc
!include Platform/ARM/SgiPkg/SgiPlatform.dsc.inc
!include Platform/ARM/SgiPkg/SgiMemoryMap2.dsc.inc

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFixedAtBuild.common]
  # GIC configurations
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x30000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x30300000
  gArmSgiTokenSpaceGuid.PcdGicSize|0x400000

  # ARM Cores and Clusters
  gArmPlatformTokenSpaceGuid.PcdCoreCount|1
  gArmPlatformTokenSpaceGuid.PcdClusterCount|16

  # RdN2Cfg3 is the third variant from RdN2 Platform
  gArmSgiTokenSpaceGuid.PcdPlatformVariant|3

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  Platform/ARM/SgiPkg/AcpiTables/RdN2Cfg3AcpiTables.inf
