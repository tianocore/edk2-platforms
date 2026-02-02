/** @file
  This is the driver that locates the MemoryConfigurationData HOB, if it
  exists, and saves the data to nvRAM.

Copyright (c) 2017 - 2022, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/CompressLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Guid/GlobalVariable.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LargeVariableReadLib.h>
#include <Library/LargeVariableWriteLib.h>
#include <Library/PcdLib.h>
#include <Library/VariableWriteLib.h>
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
  EFI_STATUS         Status;
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *HobData;
  VOID               *VariableData;
  UINTN              DataSize;
  UINTN              BufferSize;
  BOOLEAN            DataIsIdentical;
  VOID               *CompressedData;
  UINT64             CompressedSize;
  UINTN              CompressedAllocationPages;

  DataSize                  = 0;
  BufferSize                = 0;
  VariableData              = NULL;
  GuidHob                   = NULL;
  HobData                   = NULL;
  DataIsIdentical           = FALSE;
  CompressedData            = NULL;
  CompressedSize            = 0;
  CompressedAllocationPages = 0;

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

  if (PcdGetBool (PcdEnableCompressedFspNvsBuffer)) {
    if (DataSize > 0) {
      CompressedAllocationPages = EFI_SIZE_TO_PAGES (DataSize);
      CompressedData            = AllocatePages (CompressedAllocationPages);
      if (CompressedData == NULL) {
        DEBUG ((DEBUG_ERROR, "[%a] - Failed to allocate compressed data buffer.\n", __func__));
        ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
        return EFI_OUT_OF_RESOURCES;
      }

      CompressedSize = EFI_PAGES_TO_SIZE (CompressedAllocationPages);
      Status         = Compress (HobData, DataSize, CompressedData, &CompressedSize);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "[%a] - failed to compress data. Status = %r\n", __func__, Status));
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }

    HobData  = CompressedData;
    DataSize = (UINTN)CompressedSize;
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
              //
              // No need to update Variable, only lock it.
              //
              Status = LockLargeVariable (L"FspNvsBuffer",  &gFspNvsBufferVariableGuid);
              if (EFI_ERROR (Status)) {
                //
                // Fail to lock variable is security vulnerability and should not happen.
                //
                ASSERT_EFI_ERROR (Status);
                //
                // When building without ASSERT_EFI_ERROR hang, delete the variable so it will not be consumed.
                //
                DEBUG ((DEBUG_ERROR, "Delete variable!\n"));
                DataSize = 0;
                Status = SetLargeVariable (L"FspNvsBuffer", &gFspNvsBufferVariableGuid, FALSE, DataSize, HobData);
                ASSERT_EFI_ERROR (Status);
              }
            }
            FreePool (VariableData);
          }
        }
      }
      Status = EFI_SUCCESS;

      if (!DataIsIdentical) {
        Status = SetLargeVariable (L"FspNvsBuffer", &gFspNvsBufferVariableGuid, TRUE, DataSize, HobData);
        if (Status == EFI_ABORTED) {
          //
          // Fail to lock variable! This should not happen.
          //
          ASSERT_EFI_ERROR (Status);
          //
          // When building without ASSERT_EFI_ERROR hang, delete the variable so it will not be consumed.
          //
          DEBUG ((DEBUG_ERROR, "Delete variable!\n"));
          DataSize = 0;
          Status = SetLargeVariable (L"FspNvsBuffer", &gFspNvsBufferVariableGuid, FALSE, DataSize, HobData);
        }
        ASSERT_EFI_ERROR (Status);
        DEBUG ((DEBUG_INFO, "Saved size of FSP / MRC Training Data: 0x%x\n", DataSize));
      } else {
        DEBUG ((DEBUG_INFO, "FSP / MRC Training Data is identical to data from last boot, no need to save.\n"));
      }
    }
  } else {
    DEBUG((DEBUG_ERROR, "Memory S3 Data HOB was not found\n"));
  }

  if (CompressedData != NULL) {
    FreePages (CompressedData, CompressedAllocationPages);
  }

  //
  // This driver does not produce any protocol services, so always unload it.
  //
  return EFI_REQUEST_UNLOAD_IMAGE;
}
