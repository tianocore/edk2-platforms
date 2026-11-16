/** @file

  Header file for the Marvell SMC interface.

  Copyright (c) 2023 Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/* SMC function IDs for Marvell Service queries */

#define MV_SMC_ID_CALL_COUNT              0xc200ff00
#define MV_SMC_ID_UID                     0xc200ff01

#define MV_SMC_ID_VERSION                 0xc200ff03

/* x1 - node number */
#define MV_SMC_ID_DRAM_SIZE               0xc2000301

#define MV_SMC_ID_NODE_COUNT              0xc2000601

#define MV_SMC_DISABLE_RVU_LFS            0xc2000b01

#define MV_SMC_FSAFE_BOOT_SUCCESS         0xc2000b02

#define MV_SMC_SPI_SECURE_UPDATE          0xc2000b05

#define MV_SMC_SET_MAC                    0xc2000e10

#define MV_SMC_MCS_CALIBRATE              0xc2000b30

#define MV_SMC_SET_DIMM_HANDLE            0xc2000b31


#ifndef __ASSEMBLER__
UINTN SmcNodeCount (VOID);

UINTN SmcGetRamSize (IN UINTN Node);

UINTN SmcMcsCalibrate (VOID);

UINTN SmcShutdownRvu (VOID);

UINTN SmcBootSuccess (VOID);

UINTN SmcSpiSecureUpdate (IN VOID *Buffer, IN UINTN Size);

UINTN SmcSetMacAddress (IN UINT32 MacIndex, IN UINT64 MacValue);

UINTN SmcSetDimmHandle (IN UINT32 DSSIndex, IN UINT32 Handle);
#endif
