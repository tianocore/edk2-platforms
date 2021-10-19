/** @file
  This is the driver that locates the MemoryConfigurationData HOB, if it
  exists, and saves the data to nvRAM.

Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Guid/GlobalVariable.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LargeVariableReadLib.h>
#include <Library/LargeVariableWriteLib.h>
#include <Guid/FspNonVolatileStorageHob2.h>

/**
  This is the standard EFI driver point that detects whether there is a
  MemoryConfigurationData HOB and, if so, saves its data to nvRAM.

  @param[in] ImageHandle    Handle for the image of this driver
  @param[in] SystemTable    Pointer to the EFI System Table

  @retval    EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
SaveMemoryConfigEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS        Status;
  EFI_HOB_GUID_TYPE *GuidHob;
  VOID              *HobData;
  VOID              *VariableData;
  UINTN             DataSize;
  UINTN             BufferSize;
  BOOLEAN           DataIsIdentical;

  DataSize        = 0;
  BufferSize      = 0;
  VariableData    = NULL;
  GuidHob         = NULL;
  HobData         = NULL;
  DataIsIdentical = FALSE;

  //
  // Search for the Memory Configuration GUID HOB.  If it is not present, then
  // there's nothing we can do. It may not exist on the update path.
  // Firstly check version2 FspNvsHob.
  //
  GuidHob = GetFirstGuidHob (&gFspNonVolatileStorageHob2Guid);
  if (GuidHob != NULL) {
    HobData = (VOID *) (UINTN) ((FSP_NON_VOLATILE_STORAGE_HOB2 *) (UINTN) GuidHob)->NvsDataPtr;
    DataSize = (UINTN) ((FSP_NON_VOLATILE_STORAGE_HOB2 *) (UINTN) GuidHob)->NvsDataLength;
  } else {
    //
    // Fall back to version1 FspNvsHob
    //
    GuidHob = GetFirstGuidHob (&gFspNonVolatileStorageHobGuid);
    if (GuidHob != NULL) {
      HobData  = GET_GUID_HOB_DATA (GuidHob);
      DataSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
    }
  }

  if (HobData != NULL) {
    DEBUG ((DEBUG_INFO, "FspNvsHob.NvsDataLength:%d\n", DataSize));
    DEBUG ((DEBUG_INFO, "FspNvsHob.NvsDataPtr   : 0x%x\n", HobData));
    if (DataSize > 0) {
      //
      // Check if the presently saved data is identical to the data given by MRC/FSP
      //
      Status = GetLargeVariable (L"FspNvsBuffer", &gFspNvsBufferVariableGuid, &BufferSize, NULL);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        if (BufferSize == DataSize) {
          VariableData = AllocatePool (BufferSize);
          if (VariableData != NULL) {
            Status = GetLargeVariable (L"FspNvsBuffer", &gFspNvsBufferVariableGuid, &BufferSize, VariableData);
            if (!EFI_ERROR (Status) && (BufferSize == DataSize) && (0 == CompareMem (HobData, VariableData, DataSize))) {
              DataIsIdentical = TRUE;
            }
            FreePool (VariableData);
          }
        }
      }
      Status = EFI_SUCCESS;

      if (!DataIsIdentical) {
        Status = SetLargeVariable (L"FspNvsBuffer", &gFspNvsBufferVariableGuid, TRUE, DataSize, HobData);
        ASSERT_EFI_ERROR (Status);
        DEBUG ((DEBUG_INFO, "Saved size of FSP / MRC Training Data: 0x%x\n", DataSize));
      } else {
        DEBUG ((DEBUG_INFO, "FSP / MRC Training Data is identical to data from last boot, no need to save.\n"));
      }
    }
  } else {
    DEBUG((DEBUG_ERROR, "Memory S3 Data HOB was not found\n"));
  }

  //
  // This driver cannot be unloaded because DxeRuntimeVariableWriteLib constructor will register ExitBootServices callback.
  //
  return EFI_SUCCESS;
}
