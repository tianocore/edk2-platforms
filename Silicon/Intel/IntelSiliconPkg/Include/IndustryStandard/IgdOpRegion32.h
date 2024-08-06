/** @file
  IGD OpRegion definition from Intel Integrated Graphics Device OpRegion
  Specification based on version 3.2.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _IGD_OPREGION_3_2_H_
#define _IGD_OPREGION_3_2_H_

#include "IgdOpRegion30.h"

#define IGD_OPREGION_HEADER_MBOX2_VER_3_2 BIT5

#pragma pack(1)

///
/// Backlight Brightness for LFP1 and LFP2
///
typedef union {
  struct {
    UINT32 Brightness : 8;     ///< Backlight Brightness
    UINT32 Rsvd       : 22;    ///< Reserved Bit
    UINT32 Ubs        : 1;     ///< Uncalibrated Brightness Support
    UINT32 FValid     : 1;     ///< Field Valid Bit
  } Bits;
  UINT32 Data;
} IGD_BCL;

///
/// OpRegion Mailbox 2 - Backlight communication
/// Offset 0x200, Size 0x100
///
typedef struct {
  IGD_BCL BCL1;          ///< Offset 0x200 Backlight Brightness for LFP1
  IGD_BCL BCL2;          ///< Offset 0x204 Backlight Brightness for LFP2
  UINT32  CBL1;          ///< Offset 0x208 Current User Brightness Level for LFP1
  UINT32  CBL2;          ///< Offset 0x20C Current User Brightness Level for LFP2
  UINT32  BCM1[0x1E];    ///< Offset 0x210 Backlight Brightness Levels Duty Cycle Mapping Table for LFP1
  UINT32  BCM2[0x1E];    ///< Offset 0x288 Backlight Brightness Levels Duty Cycle Mapping Table for LFP2
} IGD_OPREGION_MBOX2_VER_3_2;


///
/// IGD OpRegion Structure
///
typedef struct {
  IGD_OPREGION_HEADER         Header; ///< OpRegion header (Offset 0x0, Size 0x100)
  IGD_OPREGION_MBOX1_VER_3_0  MBox1;  ///< Mailbox 1: Public ACPI Methods (Offset 0x100, Size 0x100)
  IGD_OPREGION_MBOX2_VER_3_2  MBox2;  ///< Mailbox 2: Backlight communication (Offset 0x200, Size 0x100)
  IGD_OPREGION_MBOX3_VER_3_0  MBox3;  ///< Mailbox 3: BIOS to Driver Notification (Offset 0x300, Size 0x100)
  IGD_OPREGION_MBOX4          MBox4;  ///< Mailbox 4: Video BIOS Table (VBT) (Offset 0x400, Size 0x1800)
  IGD_OPREGION_MBOX5          MBox5;  ///< Mailbox 5: BIOS to Driver Notification Extension (Offset 0x1C00, Size 0x400)
} IGD_OPREGION_STRUCTURE_VER_3_2;
#pragma pack()

#endif
