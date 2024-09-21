#
#  Copyright (c) 2025, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = RdV3R1Cfg1
  PLATFORM_GUID                  = bec32baf-6471-44f3-9ef0-a09a0cc8eb1d
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = NOOPT|DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/SgiPkg/SgiPlatform.fdf
  BOARD_DXE_FV_COMPONENTS        = Platform/ARM/SgiPkg/RdV3R1Cfg1/RdV3R1Cfg1.fdf.inc
  BUILD_NUMBER                   = 1

  DEFINE PCIE_ENABLE             = TRUE

# include common definitions from SgiPlatform.dsc
!include Platform/ARM/SgiPkg/SgiPlatform.dsc.inc
!include Platform/ARM/SgiPkg/SgiMemoryMap4.dsc.inc

# include common/basic libraries from MdePkg.
!include MdePkg/MdeLibs.dsc.inc

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFixedAtBuild.common]
  # Verbose Printing
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x00000004

  # GIC Base Addresses
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x20000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x20100000
  gArmSgiTokenSpaceGuid.PcdGicSize|0x200000

  # ARM Cores and Clusters
  gArmPlatformTokenSpaceGuid.PcdCoreCount|1
  gArmPlatformTokenSpaceGuid.PcdClusterCount|8

  # Number of chips in the multi-chip package
  gArmSgiTokenSpaceGuid.PcdChipCount|4

  # IO virtualization block
  gArmSgiTokenSpaceGuid.PcdIoVirtBlkCountPerChip|2

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  Platform/ARM/SgiPkg/AcpiTables/RdV3R1Cfg1AcpiTables.inf
