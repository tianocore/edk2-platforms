## @file
#
#  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

DEFINE BLOCK_SIZE                 = 0x1000

############################################################################
# fd total
DEFINE FD_BASE_ADDRESS            = 0x1c000000
DEFINE FD_BLOCKS                  = 0x400
DEFINE FD_SIZE                    = 0x400000

############################################################################
#flash code layout
#Set Sec base address and size in flash
DEFINE SECFV_OFFSET               = 0x00000000
DEFINE SECFV_SIZE                 = 0x00010000

#Set Pei base address and size in flash
DEFINE PEIFV_OFFSET               = 0x00010000
DEFINE PEIFV_SIZE                 = 0x00040000

#Set Dxe base address and size in flash
DEFINE DXEFV_OFFSET               = 0x00050000
DEFINE DXEFV_SIZE                 = 0x00350000
