#
#  Copyright (c) 2021, NUVIA Inc. All rights reserved.
#  Copyright (c) 2019-2024, Linaro Ltd. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  OUTPUT_DIRECTORY               = Build/SbsaQemu
  FLASH_DEFINITION               = Platform/Qemu/SbsaQemu/SbsaQemu.fdf

!include SbsaQemu-generic.dsc.inc

[PcdsFixedAtBuild.common]
  # System Memory Base -- fixed
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x10000000000
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x1000007c000
