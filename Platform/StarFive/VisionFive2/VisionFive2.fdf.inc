## @file
#  Definitions of Flash definition file on StarFive VisionFive 2 platform
#
#  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
#
# OpenSBI: 0x4000_0000 ~ 0x401F_FFFF
# EDK2   : 0x4020_0000 ~ 0x40A0_0000
#
DEFINE FSBL_SIZE      = 0x00100000 #100MB
DEFINE OPENSBI_SIZE   = 0x00200000
DEFINE FW_BASE_OFFSET = 0x40200000
DEFINE FW_SIZE        = 0x00800000 # (BLOCK_NUM x BLOCK_SIZE)

DEFINE BLOCK_NUM      = 0x800
DEFINE BLOCK_SIZE     = 0x1000

DEFINE FVMAIN_OFFSET  = 0x00000000 # Offset from FW_BASE end
DEFINE FVMAIN_SIZE    = 0x00740000

#
# EFI Variables memory region in Flash.
#
DEFINE VARS_OFFSET             = 0x00740000 # Offset from FW_BASE end
DEFINE VARS_SIZE               = 0x00040000
DEFINE VARS_FTW_WORKING_OFFSET = $(VARS_OFFSET) + $(VARS_SIZE)
DEFINE VARS_FTW_WORKING_SIZE   = 0x00040000
DEFINE VARS_FTW_SPARE_OFFSET   = $(VARS_FTW_WORKING_OFFSET) + $(VARS_FTW_WORKING_SIZE)
DEFINE VARS_FTW_SPARE_SIZE     = 0x00040000

#
# Memory map use for non-volatile variable offset in flash
# | FSBL | OPENSBI | EDK2 | EFI VARS | ... |
#
SET gJH7110TokenSpaceGuid.PcdJH7110FlashVarOffset = $(FSBL_SIZE) + $(OPENSBI_SIZE) + $(VARS_OFFSET)

#
# Temporary RAM region use for sec startup before memory is discovered
#
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPeiTempRamBase = 0x43FF0000
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPeiTempRamSize = 0x00010000

SET gUefiCpuPkgTokenSpaceGuid.PcdCpuCoreCrystalClockFrequency = 24000000 # 24Mhz
