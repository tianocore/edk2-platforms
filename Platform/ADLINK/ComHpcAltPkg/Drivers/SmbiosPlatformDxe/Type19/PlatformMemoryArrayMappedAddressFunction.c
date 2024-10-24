/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "SmbiosPlatformDxe.h"

/**
  This function adds SMBIOS Table (Type 19) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformMemoryArrayMappedAddress) {
  UINT8                Index;
  UINT8                SlotIndex;
  UINT8                MemRegionIndex;
  UINTN                HandleCount;
  UINTN                MemorySize;
  UINT16               *HandleArray;
  EFI_STATUS           Status;
  PLATFORM_DIMM        *Dimm;
  STR_TOKEN_INFO       *InputStrToken;
  PLATFORM_DIMM_LIST   *DimmList;
  PLATFORM_DRAM_INFO   *DramInfo;
  SMBIOS_TABLE_TYPE19  *InputData;
  SMBIOS_TABLE_TYPE19  *Type19Record;

  HandleCount = 0;
  HandleArray = NULL;

  GetDimmList (&DimmList);
  if (DimmList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get Dimm List\n",
      __func__,
      __LINE__
      ));
    return EFI_NOT_FOUND;
  }

  GetDramInfo (&DramInfo);
  if (DramInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get DRAM Information\n",
      __func__,
      __LINE__
      ));
    return EFI_NOT_FOUND;
  }

  SmbiosPlatformDxeGetLinkTypeHandle (
    EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,
    &HandleArray,
    &HandleCount
    );
  if (HandleArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (HandleCount != GetNumberOfSupportedSockets ()) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get Memory Array Handle\n",
      __func__,
      __LINE__
      ));
    FreePool (HandleArray);
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < GetNumberOfSupportedSockets (); Index++) {
    InputData     = (SMBIOS_TABLE_TYPE19 *)RecordData;
    InputStrToken = (STR_TOKEN_INFO *)StrToken;
    while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
      //
      // Calculate memory size
      //
      for (SlotIndex = 0; SlotIndex < DimmList->BoardDimmSlots; SlotIndex++) {
        Dimm = &DimmList->Dimm[SlotIndex];
        if (Dimm->NodeId != Index) {
          continue;
        }

        if (Dimm->Info.DimmStatus == DIMM_INSTALLED_OPERATIONAL) {
          MemorySize = Dimm->Info.DimmSize * 1024;
        }
      }

      //
      // Create Table and fill up information
      //
      for (MemRegionIndex = 0; MemRegionIndex < DramInfo->NumRegion; MemRegionIndex++) {
        SmbiosPlatformDxeCreateTable (
          (VOID *)&Type19Record,
          (VOID *)&InputData,
          sizeof (SMBIOS_TABLE_TYPE19),
          InputStrToken
          );
        if (Type19Record == NULL) {
          FreePool (HandleArray);
          return EFI_OUT_OF_RESOURCES;
        }

        if (  (DramInfo->NvdRegion[MemRegionIndex] > 0)
           || (DramInfo->Socket[MemRegionIndex] != Index))
        {
          continue;
        }

        Type19Record->ExtendedStartingAddress = DramInfo->Base[MemRegionIndex];
        Type19Record->ExtendedEndingAddress   = DramInfo->Base[MemRegionIndex] +
                                                DramInfo->Size[MemRegionIndex] -1;
        if (MemorySize != 0) {
          Type19Record->PartitionWidth = (DramInfo->Size[MemRegionIndex] - 1) / MemorySize + 1;
        }

        Type19Record->MemoryArrayHandle = HandleArray[Index];

        Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type19Record, NULL);
        if (EFI_ERROR (Status)) {
          FreePool (HandleArray);
          FreePool (Type19Record);
          return Status;
        }

        FreePool (Type19Record);
      }

      InputData++;
      InputStrToken++;
    }
  }

  FreePool (HandleArray);

  return Status;
}
