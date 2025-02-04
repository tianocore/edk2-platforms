#  @file
#
#  Platform description header for ZynqMP-based platforms
#
#  Copyright (c) 2025, Linaro Ltd. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################

!include Silicon/AMD/Xilinx/CommonPkg/CommonPkg.dsc.inc

[LibraryClasses.common]
  # ZynqMP-specific Libraries
  SerialPortLib|Silicon/AMD/Xilinx/ZynqMpPkg/Library/SerialPortLib/SerialPortLib.inf
  ArmPlatformLib|Silicon/AMD/Xilinx/ZynqMpPkg/Library/ZynqMpLib/ZynqMpLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild.common]
  # Stacks for MPCores in Normal World
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x71000000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000

  # System Memory
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x00000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x80000000

  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000

  # GIC
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0xf9020000
  gArmTokenSpaceGuid.PcdGicDistributorBase|0xf9010000

[Packages]
  Silicon/AMD/Xilinx/ZynqMpPkg/ZynqMpPkg.dec
