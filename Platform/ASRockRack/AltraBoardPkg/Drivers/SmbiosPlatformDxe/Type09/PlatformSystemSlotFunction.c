/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IOExpanderLib.h>
#include <Library/MemoryAllocationLib.h>

#include "SmbiosPlatformDxe.h"

// I2C access to the second IO Expander doesn't appear to work
#if 0

//
// IO Expander Assignment
//

#define SLIM1A_PRESENT_PIN      4
#define SLIM1B_PRESENT_PIN      5
#define SLIM2A_PRESENT_PIN      6
#define SLIM2B_PRESENT_PIN      7
#define SLIM3A_PRESENT_PIN      8
#define SLIM3B_PRESENT_PIN      9
#define SLIM4A_PRESENT_PIN     10
#define SLIM4B_PRESENT_PIN     11
#define OCU2_PRESENT_PIN       13
#define OCU1_PRESENT_PIN       14
#define PCIE4_PRESENT_X1_PIN   17
#define PCIE4_PRESENT_X4_PIN   18
#define PCIE4_PRESENT_X8_PIN   19
#define PCIE4_PRESENT_X16_PIN  20

#define PCIE5_PRESENT_X1_PIN    4
#define PCIE5_PRESENT_X4_PIN    5
#define PCIE5_PRESENT_X8_PIN    6
#define PCIE5_PRESENT_X16_PIN   7
#define PCIE6_PRESENT_X1_PIN    8
#define PCIE6_PRESENT_X4_PIN    9
#define PCIE6_PRESENT_X8_PIN   10
#define PCIE6_PRESENT_X16_PIN  11 
#define PCIE7_PRESENT_X1_PIN   13
#define PCIE7_PRESENT_X4_PIN   14
#define PCIE7_PRESENT_X8_PIN   15
#define PCIE7_PRESENT_X16_PIN  16

//
// CPU I2C Bus for IO Expander
//
#define IO_EXPANDER_I2C_BUS           4

//
// I2C address of IO Expander devices
//
#define SLIM_PCIE4_I2C_ADDRESS        32
#define PCIE5_7_I2C_ADDRESS           33

typedef enum {
  PCIE4_SLOT_INDEX = 0,
  PCIE5_SLOT_INDEX,
  PCIE6_SLOT_INDEX,
  PCIE7_SLOT_INDEX,
  OCU1_SLOT_INDEX,
  OCU2_SLOT_INDEX,
  SLIM1_SLOT_INDEX,
  SLIM2_SLOT_INDEX,
  SLIM3_SLOT_INDEX,
  SLIM4_SLOT_INDEX,
} SLOT_INDEX;

BOOLEAN
GetPinStatus (
  IN IO_EXPANDER_CONTROLLER *Controller,
  IN UINT8                  Pin
  )
{
  EFI_STATUS Status;
  UINT8      PinValue;

  Status = IOExpanderSetDir (
             Controller,
             Pin,
             CONFIG_IOEXPANDER_PIN_AS_INPUT
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to set IO pin direction\n", __func__));
    return FALSE;
  }
  Status = IOExpanderGetPin (
             Controller,
             Pin,
             &PinValue
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get IO pin value\n", __func__));
    return FALSE;
  }

  return (PinValue > 0) ? FALSE : TRUE;
}

VOID
SetIoExpanderController (
  SLOT_INDEX             SlotIndex,
  IO_EXPANDER_CONTROLLER *Controller
  )
{
  switch (SlotIndex) {
  case SLIM1_SLOT_INDEX:
  case SLIM2_SLOT_INDEX:
  case SLIM3_SLOT_INDEX:
  case SLIM4_SLOT_INDEX:
  case PCIE4_SLOT_INDEX:
  case OCU1_SLOT_INDEX:
  case OCU2_SLOT_INDEX:
    Controller->ChipID     = IO_EXPANDER_TCA9534;
    Controller->I2cBus     = IO_EXPANDER_I2C_BUS;
    Controller->I2cAddress = SLIM_PCIE4_I2C_ADDRESS;
  break;

  case PCIE5_SLOT_INDEX:
  case PCIE6_SLOT_INDEX:
  case PCIE7_SLOT_INDEX:
    Controller->ChipID = IO_EXPANDER_TCA9534;
    Controller->I2cBus = IO_EXPANDER_I2C_BUS;
    Controller->I2cAddress = PCIE5_7_I2C_ADDRESS;
  break;

  default:
    DEBUG ((DEBUG_WARN, "Warning: invalid slot index %d\n", SlotIndex));
    return;
  }
}

VOID
UpdateSmbiosType9 (
  SLOT_INDEX         SlotIndex,
  SMBIOS_TABLE_TYPE9 *InputData
  )
{
  IO_EXPANDER_CONTROLLER Controller;

  SetIoExpanderController (SlotIndex, &Controller);

  switch (SlotIndex) {
  case SLIM1_SLOT_INDEX:
    if (GetPinStatus (&Controller, SLIM1A_PRESENT_PIN) ||
        GetPinStatus (&Controller, SLIM1B_PRESENT_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case SLIM2_SLOT_INDEX:
    if (GetPinStatus (&Controller, SLIM2A_PRESENT_PIN) ||
        GetPinStatus (&Controller, SLIM2B_PRESENT_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case SLIM3_SLOT_INDEX:
    if (GetPinStatus (&Controller, SLIM3A_PRESENT_PIN) ||
        GetPinStatus (&Controller, SLIM3B_PRESENT_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case SLIM4_SLOT_INDEX:
    if (GetPinStatus (&Controller, SLIM4A_PRESENT_PIN) ||
        GetPinStatus (&Controller, SLIM4B_PRESENT_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case OCU2_SLOT_INDEX:
    if (GetPinStatus (&Controller, OCU2_PRESENT_PIN) ||
        GetPinStatus (&Controller, OCU2_PRESENT_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case OCU1_SLOT_INDEX:
    if (GetPinStatus (&Controller, OCU1_PRESENT_PIN) ||
        GetPinStatus (&Controller, OCU1_PRESENT_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case PCIE4_SLOT_INDEX:
    if (GetPinStatus (&Controller, PCIE4_PRESENT_X1_PIN) ||
        GetPinStatus (&Controller, PCIE4_PRESENT_X4_PIN) ||
        GetPinStatus (&Controller, PCIE4_PRESENT_X8_PIN) ||
        GetPinStatus (&Controller, PCIE4_PRESENT_X16_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case PCIE5_SLOT_INDEX:
    if (GetPinStatus (&Controller, PCIE5_PRESENT_X1_PIN) ||
        GetPinStatus (&Controller, PCIE5_PRESENT_X4_PIN) ||
        GetPinStatus (&Controller, PCIE5_PRESENT_X8_PIN) ||
        GetPinStatus (&Controller, PCIE5_PRESENT_X16_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case PCIE6_SLOT_INDEX:
    if (GetPinStatus (&Controller, PCIE6_PRESENT_X1_PIN) ||
        GetPinStatus (&Controller, PCIE6_PRESENT_X4_PIN) ||
        GetPinStatus (&Controller, PCIE6_PRESENT_X8_PIN) ||
        GetPinStatus (&Controller, PCIE6_PRESENT_X16_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  case PCIE7_SLOT_INDEX:
    if (GetPinStatus (&Controller, PCIE7_PRESENT_X1_PIN) ||
        GetPinStatus (&Controller, PCIE7_PRESENT_X4_PIN) ||
        GetPinStatus (&Controller, PCIE7_PRESENT_X8_PIN) ||
        GetPinStatus (&Controller, PCIE7_PRESENT_X16_PIN))
    {
      InputData->CurrentUsage = SlotUsageInUse;
    } else {
      InputData->CurrentUsage = SlotUsageAvailable;
    }
  break;

  default:
    DEBUG ((DEBUG_WARN, "Warning: unknown slot index %d\n", SlotIndex));
    return;
  }
}

#endif

/**
  This function adds SMBIOS Table (Type 9) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformSystemSlot) {
  EFI_STATUS         Status;
  // SLOT_INDEX         SlotIndex;
  STR_TOKEN_INFO     *InputStrToken;
  SMBIOS_TABLE_TYPE9 *InputData;
  SMBIOS_TABLE_TYPE9 *Type9Record;

  // SlotIndex = PCIE4_SLOT_INDEX;
  InputData = (SMBIOS_TABLE_TYPE9 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    // UpdateSmbiosType9 (SlotIndex, InputData);
    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type9Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE9),
      InputStrToken
      );
    if (Type9Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type9Record, NULL);
    FreePool (Type9Record);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    // SlotIndex++;
    InputData++;
    InputStrToken++;
  }

  return Status;
}
