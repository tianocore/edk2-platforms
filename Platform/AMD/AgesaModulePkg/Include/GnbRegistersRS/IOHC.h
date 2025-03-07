/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IOHC_H_
#define _IOHC_H_

// Bitfield Description : DBG MMIO enable.
#define DBG_BASE_ADDR_LO_DBG_MMIO_EN_OFFSET  0

// Bitfield Description : Locks the DBG MMIO address range and enable until the next warm reset.
#define DBG_BASE_ADDR_LO_DBG_MMIO_LOCK_OFFSET  1

#define SMN_IOHUB0NBIO0_IOAPIC_BASE_ADDR_LO_ADDRESS   0x13b102f0UL
#define IOAPIC_BASE_ADDR_LO_IOAPIC_BASE_ADDR_LO_MASK  0xffffff00
#define SMN_IOHUB1NBIO0_IOAPIC_BASE_ADDR_LO_ADDRESS   0x13c102f0UL

#endif /* _IOHC_H_ */
