/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_INIT_DXE_H_
#define PLATFORM_INIT_DXE_H_

#define FAILSAFE_BOOT_NORMAL               0x00
#define FAILSAFE_BOOT_LAST_KNOWN_SETTINGS  0x01
#define FAILSAFE_BOOT_DEFAULT_SETTINGS     0x02
#define FAILSAFE_BOOT_DDR_DOWNGRADE        0x03
#define FAILSAFE_BOOT_SUCCESSFUL           0x04

#pragma pack(1)
typedef struct {
  UINT8     ImgMajorVer;
  UINT8     ImgMinorVer;
  UINT32    NumRetry1;
  UINT32    NumRetry2;
  UINT32    MaxRetry;
  UINT8     Status;
  //
  // Byte[3]: Reserved
  // Byte[2]: Slave MCU Failure Mask
  // Byte[1]: Reserved
  // Byte[0]: Master MCU Failure Mask
  //
  UINT32    MCUFailsMask;
  UINT16    CRC16;
  UINT8     Reserved[3];
} FAIL_SAFE_CONTEXT;
#pragma pack()

#endif /* PLATFORM_INIT_DXE_H_ */
