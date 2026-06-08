/** @file
  Implementation of FirmwareInterfaceTableCacheLib.

  Caches the FIT (Firmware Interface Table) into a GUIDed HOB during
  pre-memory init so post-memory consumers avoid top-of-flash access.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/FirmwareInterfaceTableCacheLib.h>

/**
  Cache the FIT table entries into a GUIDed HOB so that post-memory
  consumers can retrieve FIT records without accessing the top-of-flash
  region directly.

  @retval EFI_SUCCESS           FIT entries cached successfully.
  @retval EFI_NOT_FOUND         FIT pointer is invalid or signature not found.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the HOB.
**/
EFI_STATUS
EFIAPI
BuildFitEntryHob (
  VOID
  )
{
  FIRMWARE_INTERFACE_TABLE_ENTRY  *FitBase;
  UINT32                         EntryCount;
  UINT32                         MaxEntries;
  UINTN                          HobDataSize;
  FIT_ENTRY_HOB_DATA           *HobData;

  FitBase = (FIRMWARE_INTERFACE_TABLE_ENTRY *)(UINTN)(*(UINTN *)(UINTN) FIT_POINTER_ADDRESS);

  if ((UINTN) FitBase == (UINTN) 0xFFFFFFFF) {
    DEBUG ((DEBUG_WARN, "BuildFitEntryHob: Invalid FIT pointer\n"));
    return EFI_NOT_FOUND;
  }

  if (FitBase->Address != FIT_TYPE_00_SIGNATURE) {
    DEBUG ((DEBUG_WARN, "BuildFitEntryHob: FIT signature not found\n"));
    return EFI_NOT_FOUND;
  }

  EntryCount = (UINT32) FitBase->Size[0]
             | ((UINT32) FitBase->Size[1] << 8)
             | ((UINT32) FitBase->Size[2] << 16);

  MaxEntries = (MAX_UINT16 - sizeof (EFI_HOB_GUID_TYPE) - sizeof (FIT_ENTRY_HOB_DATA))
             / sizeof (FIRMWARE_INTERFACE_TABLE_ENTRY);
  if (EntryCount > MaxEntries) {
    EntryCount = MaxEntries;
  }

  HobDataSize = sizeof (FIT_ENTRY_HOB_DATA) + EntryCount * sizeof (FIRMWARE_INTERFACE_TABLE_ENTRY);
  HobData = BuildGuidHob (&gFitEntryHobGuid, HobDataSize);
  if (HobData == NULL) {
    DEBUG ((DEBUG_ERROR, "BuildFitEntryHob: Failed to build HOB\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  HobData->EntryCount = EntryCount;
  CopyMem (HobData->Entry, FitBase, EntryCount * sizeof (FIRMWARE_INTERFACE_TABLE_ENTRY));

  DEBUG ((DEBUG_INFO, "BuildFitEntryHob: Cached %d FIT entries\n", EntryCount));
  return EFI_SUCCESS;
}
