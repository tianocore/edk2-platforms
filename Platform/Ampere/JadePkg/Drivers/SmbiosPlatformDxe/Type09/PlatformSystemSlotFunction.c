/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IOExpanderLib.h>
#include <Library/MemoryAllocationLib.h>

#include "SmbiosPlatformDxe.h"

//
// IO Expander Assignment
//
#define S0_RISERX32_SLOT1_PRESENT_PIN1  12   /* P12: S0_PCIERCB_3A_PRSNT_1C_L */
#define S0_RISERX32_SLOT1_PRESENT_PIN2  13   /* P13: S0_PCIERCB_3A_PRSNT_2C_L */
#define S0_RISERX32_SLOT2_PRESENT_PIN1  4    /* P04: S0_PCIERCA3_PRSNT_1C_L   */
#define S0_RISERX32_SLOT2_PRESENT_PIN2  5    /* P05: S0_PCIERCA3_PRSNT_2C_L   */
#define S0_RISERX32_SLOT2_PRESENT_PIN3  6    /* P06: S0_PCIERCA3_PRSNT_4C_L   */
#define S0_RISERX32_SLOT3_PRESENT_PIN1  10   /* P10: S0_PCIERCB_2B_PRSNT_1C_L */
#define S0_RISERX32_SLOT3_PRESENT_PIN2  11   /* P11: S0_PCIERCB_2B_PRSNT_2C_L */
#define S1_RISERX24_SLOT1_PRESENT_PIN1  0    /* P00: S1_PCIERCB1B_PRSNT_L     */
#define S1_RISERX24_SLOT1_PRESENT_PIN2  1    /* P01: S1_PCIERCB1B_PRSNT_1C_L  */
#define S1_RISERX24_SLOT2_PRESENT_PIN   3    /* P03: S1_PCIERCA3_2_PRSNT_4C_L */
#define S1_RISERX24_SLOT3_PRESENT_PIN   2    /* P02: S1_PCIERCA3_1_PRSNT_2C_L */
#define S1_RISERX8_SLOT1_PRESENT_PIN1   4    /* P04: S1_PCIERCB0A_PRSNT_2C_L  */
#define S1_RISERX8_SLOT1_PRESENT_PIN2   5    /* P05: S1_PCIERCB0A_PRSNT_L     */
#define S0_OCP_SLOT_PRESENT_PIN1        0    /* P00: OCP_PRSNTB0_L            */
#define S0_OCP_SLOT_PRESENT_PIN2        1    /* P01: OCP_PRSNTB1_L            */
#define S0_OCP_SLOT_PRESENT_PIN3        2    /* P02: OCP_PRSNTB2_L            */
#define S0_OCP_SLOT_PRESENT_PIN4        3    /* P03: OCP_PRSNTB3_D            */

//
// CPU I2C Bus for IO Expander
//
#define S0_RISER_I2C_BUS  0x06
#define S0_OCP_I2C_BUS    0x06
#define S1_RISER_I2C_BUS  0x0F

//
// I2C address of IO Expander devices
//
#define S0_RISERX32_I2C_ADDRESS  0x22
#define S1_RISERX24_I2C_ADDRESS  0x22
#define S1_RISERX8_I2C_ADDRESS   0x22
#define S0_OCP_I2C_ADDRESS       0x20

typedef enum {
  S0_RISERX32_SLOT1_INDEX = 0,
  S0_RISERX32_SLOT2_INDEX,
  S0_RISERX32_SLOT3_INDEX,
  S1_RISERX24_SLOT1_INDEX,
  S1_RISERX24_SLOT2_INDEX,
  S1_RISERX24_SLOT3_INDEX,
  S1_RISERX8_SLOT1_INDEX,
  S0_OCP_NIC_3_0_INDEX,
  S0_NVME_M_2_SLOT1_INDEX,
  S0_NVME_M_2_SLOT2_INDEX
} SLOT_INDEX;

BOOLEAN
GetPinStatus (
  IN IO_EXPANDER_CONTROLLER  *Controller,
  IN UINT8                   Pin
  )
{
  EFI_STATUS  Status;
  UINT8       PinValue;

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
  SLOT_INDEX              SlotIndex,
  IO_EXPANDER_CONTROLLER  *Controller
  )
{
  switch (SlotIndex) {
    case S0_RISERX32_SLOT1_INDEX:
    case S0_RISERX32_SLOT2_INDEX:
    case S0_RISERX32_SLOT3_INDEX:
      Controller->ChipID     = IO_EXPANDER_TCA6424A;
      Controller->I2cBus     = S0_RISER_I2C_BUS;
      Controller->I2cAddress = S0_RISERX32_I2C_ADDRESS;
      break;

    case S0_OCP_NIC_3_0_INDEX:
      Controller->ChipID     = IO_EXPANDER_TCA9534;
      Controller->I2cBus     = S0_OCP_I2C_BUS;
      Controller->I2cAddress = S0_OCP_I2C_ADDRESS;
      break;

    case S1_RISERX24_SLOT1_INDEX:
    case S1_RISERX24_SLOT2_INDEX:
    case S1_RISERX24_SLOT3_INDEX:
      Controller->ChipID     = IO_EXPANDER_TCA6424A;
      Controller->I2cBus     = S1_RISER_I2C_BUS;
      Controller->I2cAddress = S1_RISERX24_I2C_ADDRESS;
      break;

    case S1_RISERX8_SLOT1_INDEX:
      Controller->ChipID     = IO_EXPANDER_TCA6424A;
      Controller->I2cBus     = S1_RISER_I2C_BUS;
      Controller->I2cAddress = S1_RISERX8_I2C_ADDRESS;
      break;

    default:
      return;
  }
}

VOID
UpdateSmbiosType9 (
  SLOT_INDEX          SlotIndex,
  SMBIOS_TABLE_TYPE9  *InputData
  )
{
  IO_EXPANDER_CONTROLLER  Controller;

  SetIoExpanderController (SlotIndex, &Controller);

  switch (SlotIndex) {
    case S0_RISERX32_SLOT1_INDEX:
      if (  GetPinStatus (&Controller, S0_RISERX32_SLOT1_PRESENT_PIN1)
         || GetPinStatus (&Controller, S0_RISERX32_SLOT1_PRESENT_PIN2))
      {
        InputData->CurrentUsage = SlotUsageInUse;
      } else {
        InputData->CurrentUsage = SlotUsageAvailable;
      }

      //
      // According to Mt.Jade schematic for Altra Max, PCIe block diagram,
      // system slots connect to different Root Complex when we compare with
      // Mt.Jade schematic for Altra, so the segment group is changed.
      //
      //
      // PCI Segment Groups defined in the system via the _SEG object in the ACPI name space.
      // Refer to AC02 ACPI tables to check the fields below.
      //
      if (!IsAc01Processor ()) {
        // Socket 0 RCA7
        InputData->SegmentGroupNum = 5;
      }

      break;

    case S0_RISERX32_SLOT2_INDEX:
      if (  GetPinStatus (&Controller, S0_RISERX32_SLOT2_PRESENT_PIN1)
         || GetPinStatus (&Controller, S0_RISERX32_SLOT2_PRESENT_PIN2)
         || GetPinStatus (&Controller, S0_RISERX32_SLOT2_PRESENT_PIN3))
      {
        InputData->CurrentUsage = SlotUsageInUse;
      } else {
        InputData->CurrentUsage = SlotUsageAvailable;
      }

      if (!IsAc01Processor ()) {
        // Socket 0 RCA3
        InputData->SegmentGroupNum = 0;
      }

      break;

    case S0_RISERX32_SLOT3_INDEX:
      if (  GetPinStatus (&Controller, S0_RISERX32_SLOT3_PRESENT_PIN1)
         || GetPinStatus (&Controller, S0_RISERX32_SLOT3_PRESENT_PIN2))
      {
        InputData->CurrentUsage = SlotUsageInUse;
      } else {
        InputData->CurrentUsage = SlotUsageAvailable;
      }

      if (!IsAc01Processor ()) {
        // Socket 0 RCA7
        InputData->SegmentGroupNum = 5;
      }

      break;

    case S0_OCP_NIC_3_0_INDEX:
      if (  GetPinStatus (&Controller, S0_OCP_SLOT_PRESENT_PIN1)
         || GetPinStatus (&Controller, S0_OCP_SLOT_PRESENT_PIN2)
         || GetPinStatus (&Controller, S0_OCP_SLOT_PRESENT_PIN3)
         || GetPinStatus (&Controller, S0_OCP_SLOT_PRESENT_PIN4))
      {
        InputData->CurrentUsage = SlotUsageInUse;
      } else {
        InputData->CurrentUsage = SlotUsageAvailable;
      }

      if (!IsAc01Processor ()) {
        // Socket 0 RCA2
        InputData->SegmentGroupNum = 1;
      }

      break;

    case S1_RISERX24_SLOT1_INDEX:
      if (IsSlaveSocketActive ()) {
        if (  GetPinStatus (&Controller, S1_RISERX24_SLOT1_PRESENT_PIN1)
           || GetPinStatus (&Controller, S1_RISERX24_SLOT1_PRESENT_PIN2))
        {
          InputData->CurrentUsage = SlotUsageInUse;
        } else {
          InputData->CurrentUsage = SlotUsageAvailable;
        }
      }

      if (!IsAc01Processor ()) {
        // Socket 1 RCA4
        InputData->SegmentGroupNum = 8;
      }

      break;

    case S1_RISERX24_SLOT2_INDEX:
      if (IsSlaveSocketActive ()) {
        InputData->CurrentUsage =
          GetPinStatus (&Controller, S1_RISERX24_SLOT2_PRESENT_PIN) ? SlotUsageInUse : SlotUsageAvailable;
      }

      if (!IsAc01Processor ()) {
        // Socket 1 RCA3
        InputData->SegmentGroupNum = 7;
      }

      break;

    case S1_RISERX24_SLOT3_INDEX:
      if (IsSlaveSocketActive ()) {
        InputData->CurrentUsage =
          GetPinStatus (&Controller, S1_RISERX24_SLOT3_PRESENT_PIN) ? SlotUsageInUse : SlotUsageAvailable;
      }

      if (!IsAc01Processor ()) {
        // Socket 1 RCA3
        InputData->SegmentGroupNum = 7;
      }

      break;

    case S1_RISERX8_SLOT1_INDEX:
      if (IsSlaveSocketActive ()) {
        if (  GetPinStatus (&Controller, S1_RISERX8_SLOT1_PRESENT_PIN1)
           || GetPinStatus (&Controller, S1_RISERX8_SLOT1_PRESENT_PIN2))
        {
          InputData->CurrentUsage = SlotUsageInUse;
        } else {
          InputData->CurrentUsage = SlotUsageAvailable;
        }
      }

      if (!IsAc01Processor ()) {
        // Socket 1 RCA4
        InputData->SegmentGroupNum = 8;
      }

      break;

    case S0_NVME_M_2_SLOT1_INDEX:
      if (!IsAc01Processor ()) {
        // Socket 0 RCA6
        InputData->SegmentGroupNum = 4;
      }

      break;

    case S0_NVME_M_2_SLOT2_INDEX:
      if (!IsAc01Processor ()) {
        // Socket 0 RCA6
        InputData->SegmentGroupNum = 4;
      }

      break;

    default:
      return;
  }
}

/**
  This function adds SMBIOS Table (Type 9) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformSystemSlot) {
  EFI_STATUS          Status;
  SLOT_INDEX          SlotIndex;
  STR_TOKEN_INFO      *InputStrToken;
  SMBIOS_TABLE_TYPE9  *InputData;
  SMBIOS_TABLE_TYPE9  *Type9Record;

  SlotIndex     = S0_RISERX32_SLOT1_INDEX;
  InputData     = (SMBIOS_TABLE_TYPE9 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    UpdateSmbiosType9 (SlotIndex, InputData);
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
    if (EFI_ERROR (Status)) {
      FreePool (Type9Record);
      return Status;
    }

    FreePool (Type9Record);
    SlotIndex++;
    InputData++;
    InputStrToken++;
  }

  return Status;
}
