/** @file
  The instance of Post Code Library that layers on top of a Debug Library instance.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/PL011UartLib.h>
#include <Library/MmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/NVParamLib.h>
#include <NVParamDef.h>

UINT8
GetFirmwareMajorVersion (
  )
{
  UINT16      ACLRd = NV_PERM_ALL;
  EFI_STATUS  Status;
  UINT32      Val;

  Status = NVParamGet (NV_SI_RO_BOARD_I2C_VRD_CONFIG_INFO, ACLRd, &Val);

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, " I2C brd config info    0x%X (%d)\n", Val, Val));
  }

  if (Val == 0x6A685860) {
    return 0xA2;
  } else if (Val == 0x6A687F60) {
    return 0xA1;
  } else {
    return 0xA0;
  }
}
