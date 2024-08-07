/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2023 Marvell
*
* Header file for for Marvell SMC Interface
*
**/

#ifndef SMCLIB_H__
#define SMCLIB_H__

/* SMC function IDs for Marvell Service queries */

#define MV_SMC_ID_CALL_COUNT              0xc200ff00
#define MV_SMC_ID_UID                     0xc200ff01

#define MV_SMC_ID_VERSION                 0xc200ff03

/* x1 - node number */
#define MV_SMC_ID_DRAM_SIZE               0xc2000301


UINTN SmcGetRamSize (IN UINTN Node);

#endif
