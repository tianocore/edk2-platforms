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
[Defines]
  OUTPUT_DIRECTORY               = Build/SbsaQemu-rme
  FLASH_DEFINITION               = Platform/Qemu/SbsaQemu/SbsaQemu-rme.fdf

!include SbsaQemu-generic.dsc.inc

[PcdsFixedAtBuild.common]
  # System Memory Base -- when RME is enabled, the memory base is set
  # after the RMM
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x10043000000
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x1004307c000
