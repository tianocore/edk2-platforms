/** @file
  SMBIOS Type 9 (System Slots) tables for the Marvell CN10K SoC.

  NOTE: These tables are valid only for the ebb106s evaluation board and are
  installed only when that board is detected. Treat them as a reference
  implementation for other boards.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE9  mSmbiosType9Slot1Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_SLOTS, sizeof (SMBIOS_TABLE_TYPE9), 0 },
  1,                    // SlotDesignation String
  0xBE,                 // SlotType;    0xBE = PCI Express Gen 5, Value from MISC_SLOT_TYPE.
  SlotDataBusWidth4X,   // SlotDataBusWidth; Value from MISC_SLOT_DATA_BUS_WIDTH.
  SlotUsageUnknown,     // CurrentUsage; Value from MISC_SLOT_USAGE.
  SlotLengthOther,      // SlotLength; Value from MISC_SLOT_LENGTH.
  0,                    // SlotID;
  {     // SlotCharacteristics1;
    1,  // CharacteristicsUnknown  :1;
    0,  // Provides50Volts         :1;
    0,  // Provides33Volts         :1;
    0,  // SharedSlot              :1;
    0,  // PcCard16Supported       :1;
    0,  // CardBusSupported        :1;
    0,  // ZoomVideoSupported      :1;
    0,  // ModemRingResumeSupported:1;
  },
  {     // SlotCharacteristics2;
    0,  // PmeSignalSupported      :1;
    0,  // HotPlugDevicesSupported :1;
    0,  // SmbusSignalSupported    :1;
    0,  // Reserved                :5;  ///< Set to 0.
  },
  0,    // SegmentGroupNum;
  0,    // BusNum;
  0,    // DevFuncNum;
};

STATIC CHAR8  *mSmbiosType9Strings1[] = { "J4: PCIe Slot1 GSERP0 x4/2", NULL };

STATIC SMBIOS_TABLE_TYPE9  mSmbiosType9Slot2Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_SLOTS, sizeof (SMBIOS_TABLE_TYPE9), 0 },
  1,                    // SlotDesignation String
  0xBE,                 // SlotType;    0xBE = PCI Express Gen 5, Value from MISC_SLOT_TYPE.
  SlotDataBusWidth2X,   // SlotDataBusWidth; Value from MISC_SLOT_DATA_BUS_WIDTH.
  SlotUsageUnknown,     // CurrentUsage; Value from MISC_SLOT_USAGE.
  SlotLengthOther,      // SlotLength; Value from MISC_SLOT_LENGTH.
  0,                    // SlotID;
  {     // SlotCharacteristics1;
    1,  // CharacteristicsUnknown  :1;
    0,  // Provides50Volts         :1;
    0,  // Provides33Volts         :1;
    0,  // SharedSlot              :1;
    0,  // PcCard16Supported       :1;
    0,  // CardBusSupported        :1;
    0,  // ZoomVideoSupported      :1;
    0,  // ModemRingResumeSupported:1;
  },
  {     // SlotCharacteristics2;
    0,  // PmeSignalSupported      :1;
    0,  // HotPlugDevicesSupported :1;
    0,  // SmbusSignalSupported    :1;
    0,  // Reserved                :5;  ///< Set to 0.
  },
  0,    // SegmentGroupNum;
  0,    // BusNum;
  0,    // DevFuncNum;
};

STATIC CHAR8  *mSmbiosType9Strings2[] = { "J6: PCIe Slot2 GSERP0 x2", NULL };

STATIC SMBIOS_TABLE_TYPE9  mSmbiosType9Slot3Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_SLOTS, sizeof (SMBIOS_TABLE_TYPE9), 0 },
  1,                    // SlotDesignation String
  0xBE,                 // SlotType;    0xBE = PCI Express Gen 5, Value from MISC_SLOT_TYPE.
  SlotDataBusWidth2X,   // SlotDataBusWidth; Value from MISC_SLOT_DATA_BUS_WIDTH.
  SlotUsageUnknown,     // CurrentUsage; Value from MISC_SLOT_USAGE.
  SlotLengthOther,      // SlotLength; Value from MISC_SLOT_LENGTH.
  0,                    // SlotID;
  {     // SlotCharacteristics1;
    1,  // CharacteristicsUnknown  :1;
    0,  // Provides50Volts         :1;
    0,  // Provides33Volts         :1;
    0,  // SharedSlot              :1;
    0,  // PcCard16Supported       :1;
    0,  // CardBusSupported        :1;
    0,  // ZoomVideoSupported      :1;
    0,  // ModemRingResumeSupported:1;
  },
  {     // SlotCharacteristics2;
    0,  // PmeSignalSupported      :1;
    0,  // HotPlugDevicesSupported :1;
    0,  // SmbusSignalSupported    :1;
    0,  // Reserved                :5;  ///< Set to 0.
  },
  0,    // SegmentGroupNum;
  0,    // BusNum;
  0,    // DevFuncNum;
};

STATIC CHAR8  *mSmbiosType9Strings3[] = { "J3: PCIe Slot3 GSERP1 x2", NULL };

typedef struct {
  SMBIOS_TABLE_TYPE9    *Template;
  CHAR8                 **Strings;
} TYPE9_ENTRY;

STATIC TYPE9_ENTRY  mSmbiosType9Entries[] = {
  { &mSmbiosType9Slot1Template, mSmbiosType9Strings1 },
  { &mSmbiosType9Slot2Template, mSmbiosType9Strings2 },
  { &mSmbiosType9Slot3Template, mSmbiosType9Strings3 },
};

/**
  Install the SMBIOS Type 9 (System Slots) records.

  Adds one PCI Express Gen 5 record for each of the three ebb106s slots,
  including the slot designation and lane width.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context. Unused.

  @retval  EFI_SUCCESS  All slot records were installed.
  @retval  Others       A record could not be added.
**/
EFI_STATUS
InstallType9SystemSlots (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  for (Index = 0; Index < ARRAY_SIZE (mSmbiosType9Entries); Index++) {
    Status = SmbiosPlatformAddEntry (
               Smbios,
               (SMBIOS_STRUCTURE *)mSmbiosType9Entries[Index].Template,
               mSmbiosType9Entries[Index].Strings,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
