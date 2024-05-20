/** @file
  AMD SMBIOS Type 9 Record

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/PrintLib.h>
#include "SmbiosCommon.h"
#include <Library/AmdPlatformSocLib.h>

/**
  This function checks for system slot info and adds smbios record (Type 9).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.

**/
EFI_STATUS
EFIAPI
SystemSlotInfoFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                   Status;
  EFI_SMBIOS_HANDLE            SmbiosHandle;
  SMBIOS_TABLE_TYPE9           *SmbiosRecord;
  UINTN                        SystemSlotCount;
  SMBIOS_TABLE_TYPE9           *SystemSlotInfo;
  UINTN                        Index;
  CHAR8                        SlotDesignationStr[SMBIOS_STRING_MAX_LENGTH];
  SMBIOS_TABLE_TYPE9_EXTENDED  SmbiosRecordExtended;
  UINTN                        SlotDesStrLen;
  UINTN                        TotalSize;

  if (Smbios == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SystemSlotInfo  = NULL;
  SystemSlotCount = 0;
  // Invoke GetSystemSlotInfo function to get the number of system slots.
  Status = GetSystemSlotInfo (&SystemSlotInfo, &SystemSlotCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < SystemSlotCount; Index++) {
    SlotDesStrLen = AsciiSPrint (
                      SlotDesignationStr,
                      SMBIOS_STRING_MAX_LENGTH,
                      "PCIE-%d",
                      SystemSlotInfo[Index].SlotID
                      );
    // Two zeros following the last string.
    TotalSize    = sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED) + SlotDesStrLen + 2;
    SmbiosRecord = NULL;
    SmbiosRecord = AllocateZeroPool (TotalSize);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      CopyMem (SmbiosRecord, &SystemSlotInfo[Index], sizeof (SMBIOS_TABLE_TYPE9));
      SmbiosRecord->Hdr.Type                 = SMBIOS_TYPE_SYSTEM_SLOTS;
      SmbiosRecord->Hdr.Length               = sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED);
      SmbiosRecord->Hdr.Handle               = 0;
      SmbiosRecord->SlotDesignation          = 1;
      SmbiosRecordExtended.SlotHeight        = SlotHeightUnknown;
      SmbiosRecordExtended.SlotPitch         = 0;
      SmbiosRecordExtended.SlotPhysicalWidth = SmbiosRecord->SlotDataBusWidth;
      CopyMem (&SmbiosRecord->SlotCharacteristics1, PcdGetPtr (PcdAmdSmbiosType9SlotCharacteristics1), sizeof (MISC_SLOT_CHARACTERISTICS1));
      CopyMem (&SmbiosRecord->SlotCharacteristics2, PcdGetPtr (PcdAmdSmbiosType9SlotCharacteristics2), sizeof (MISC_SLOT_CHARACTERISTICS2));
      CopyMem ((UINT8 *)SmbiosRecord->PeerGroups + SmbiosRecord->PeerGroupingCount * sizeof (SmbiosRecord->PeerGroups), (UINT8 *)&SmbiosRecordExtended, sizeof (SMBIOS_TABLE_TYPE9_EXTENDED));
      CopyMem ((UINT8 *)SmbiosRecord + SmbiosRecord->Hdr.Length, SlotDesignationStr, SlotDesStrLen);
      Status = AddCommonSmbiosRecord (
                 Smbios,
                 &SmbiosHandle,
                 (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
                 );
      if (EFI_ERROR (Status)) {
        FreePool (SmbiosRecord);
        break;
      }

      if (SmbiosRecord != NULL) {
        FreePool (SmbiosRecord);
      }
    }
  }

  FreePool (SystemSlotInfo);
  return EFI_SUCCESS;
}
