/** @file
  Boot service PEI BIOS ID library implementation.

Copyright (c) 2015 - 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BiosIdLib.h>

#include <Guid/BiosId.h>

/**
  This function returns BIOS ID by searching HOB or FV.
  It also debug print the BIOS ID found.

  @param[out] BiosIdImage   The BIOS ID got from HOB or FV. It is optional,
                            no BIOS ID will be returned if it is NULL as input.

  @retval EFI_SUCCESS               BIOS ID has been got successfully.
  @retval EFI_NOT_FOUND             BIOS ID image is not found, and no parameter will be modified.

**/
EFI_STATUS
EFIAPI
GetBiosId (
  OUT BIOS_ID_IMAGE  *BiosIdImage OPTIONAL
  )
{
  EFI_STATUS           Status;
  BIOS_ID_IMAGE        TempBiosIdImage;
  VOID                 *Address;
  UINTN                Size;
  UINTN                Instance;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;

  Address = NULL;
  Size    = 0;

  if (BiosIdImage == NULL) {
    //
    // It is NULL as input, so no BIOS ID will be returned.
    // Use temp buffer to hold the BIOS ID.
    //
    BiosIdImage = &TempBiosIdImage;
  }

  Address = GetFirstGuidHob (&gBiosIdGuid);
  if (Address != NULL) {
    Size = sizeof (BIOS_ID_IMAGE);
    CopyMem ((VOID *)BiosIdImage, GET_GUID_HOB_DATA (Address), Size);

    DEBUG ((DEBUG_INFO, "PEI get BIOS ID from HOB successfully\n"));
    DEBUG ((DEBUG_INFO, "BIOS ID: %s\n", (CHAR16 *)(&(BiosIdImage->BiosIdString))));
    return EFI_SUCCESS;
  }

  VolumeHandle = NULL;
  Instance     = 0;
  while (TRUE) {
    //
    // Traverse all firmware volume instances.
    //
    Status = PeiServicesFfsFindNextVolume (Instance, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    FileHandle = NULL;
    Status     = PeiServicesFfsFindFileByName (&gBiosIdGuid, VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      //
      // Search RAW section.
      //
      Status = PeiServicesFfsFindSectionData (EFI_SECTION_RAW, FileHandle, &Address);
      if (!EFI_ERROR (Status)) {
        //
        // BIOS ID image is present in this FV.
        //
        Size = sizeof (BIOS_ID_IMAGE);
        CopyMem ((VOID *)BiosIdImage, Address, Size);

        DEBUG ((DEBUG_INFO, "PEI get BIOS ID from FV successfully\n"));
        DEBUG ((DEBUG_INFO, "BIOS ID: %s\n", (CHAR16 *)(&(BiosIdImage->BiosIdString))));

        //
        // Build GUID HOB for the BIOS ID image.
        //
        BuildGuidDataHob (&gBiosIdGuid, Address, Size);
        return EFI_SUCCESS;
      }
    }

    //
    // Search the next volume.
    //
    Instance++;
  }

  DEBUG ((DEBUG_ERROR, "PEI get BIOS ID failed: %r\n", EFI_NOT_FOUND));
  return EFI_NOT_FOUND;
}
