/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "SmbiosBoardSpecificDxe.h"

/**
  This function adds SMBIOS Table (Type 9) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_BOARD_SPECIFIC_DXE_TABLE_FUNCTION (PlatformSystemSlot) {
  EFI_STATUS         Status;
  STR_TOKEN_INFO     *InputStrToken;
  SMBIOS_TABLE_TYPE9 *InputData;
  SMBIOS_TABLE_TYPE9 *Type9Record;

  InputData = (SMBIOS_TABLE_TYPE9 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {

    // We set the bus width to Unknown for slots which differ between Altra
    // and Altra Max
    if (InputData->SlotDataBusWidth == SlotDataBusWidthUnknown) {
      if (IsAc01Processor ()) {
        // AC01 has 2 x16 slots and 2 x8.
        InputData->SlotDataBusWidth = SlotDataBusWidth8X;
      } else {
        // AC02 has 4 x16 slots
        InputData->SlotDataBusWidth = SlotDataBusWidth16X;
      }
    }

    SmbiosBoardSpecificDxeCreateTable (
      (VOID *)&Type9Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE9),
      InputStrToken
      );
    if (Type9Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosBoardSpecificDxeAddRecord ((UINT8 *)Type9Record, NULL);
    FreePool (Type9Record);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    InputData++;
    InputStrToken++;
  }

  return Status;
}
