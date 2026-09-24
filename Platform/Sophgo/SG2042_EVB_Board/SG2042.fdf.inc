## @file SG2042-EVB.fdf.inc
#
#  Definitions of Flash definition file on SOPHGO SG2042-EVB RISC-V platform
#
#  Copyright (c) 2025, SOPHGO Inc. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
DEFINE BLOCK_SIZE        = 0x1000
DEFINE FW_BASE_ADDRESS   = 0x02000000
DEFINE FW_SIZE           = 0x00e00000
DEFINE FW_BLOCKS         = 0xe00

DEFINE EFI_MEMORY_SIZE    = 64 * 1024 * 1024
DEFINE EFI_STACK_SIZE     = 64 * 1024
DEFINE EFI_MEMORY_BOTTOM  = $(FW_BASE_ADDRESS) + $(FW_SIZE)
DEFINE EFI_MEMORY_TOP     = $(EFI_MEMORY_BOTTOM) + $(EFI_MEMORY_SIZE)
DEFINE EFI_STACK_BASE     = $(EFI_MEMORY_TOP) - $(EFI_STACK_SIZE)

#
# 0x000000-0x7FFFFF code
# 0x800000-0x82FFFF variables
#
DEFINE CODE_BASE_ADDRESS = $(FW_BASE_ADDRESS)
DEFINE CODE_SIZE         = 0x00800000
DEFINE CODE_BLOCKS       = 0x800
DEFINE VARS_BLOCKS       = 0x30

#
# Other FV regions are in the second FW domain.
# The size of memory region must be power of 2.
# The base address must be aligned with the size.
#
# FW memory region
#
DEFINE FVMAIN_OFFSET                 = 0x00000000
DEFINE FVMAIN_SIZE                   = 0x00800000

#
# EFI Variable memory region.
# The total size of EFI Variable FD must include
# all of sub regions of EFI Variable
#
DEFINE VARS_OFFSET                   = 0x00800000
DEFINE VARS_SIZE                     = 0x00010000
DEFINE VARS_FTW_WORKING_OFFSET       = $(VARS_OFFSET) + $(VARS_SIZE)
DEFINE VARS_FTW_WORKING_SIZE         = 0x00010000
DEFINE VARS_FTW_SPARE_OFFSET         = $(VARS_FTW_WORKING_OFFSET) + $(VARS_FTW_WORKING_SIZE)
DEFINE VARS_FTW_SPARE_SIZE           = 0x00010000

SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFdBlockSize               = $(BLOCK_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamBase                  = $(EFI_STACK_BASE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamSize                  = $(EFI_STACK_SIZE)
