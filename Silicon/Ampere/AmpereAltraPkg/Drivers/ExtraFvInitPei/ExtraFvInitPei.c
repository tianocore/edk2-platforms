/** @file

  Copyright (c) 2025, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h> // AllocatePool()
#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FlashLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>

/**
  Get FV image from the Uefi-Extra region, then install FV INFO(2) PPI, Build FV HOB.

  @param  FileHandle              Handle of the file being invoked. Type
                                  EFI_PEI_FILE_HANDLE is defined in
                                  FfsFindNextFile().
  @param  PeiServices             Describes the list of possible PEI Services.

  @retval EFI_SUCCESS             The entry point was executed successfully.
  @retval EFI_OUT_OF_RESOURCES    There is not enough memory to complete the
                                  operations.

**/
EFI_STATUS
EFIAPI
InitializeExtraFvInitPeim (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  UINT32                      UefiExtraFdSize;
  UINT64                      UefiExtraFdBaseAddress;
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;

  UefiExtraFdSize        = PcdGet32 (PcdUefiExtraFdSize);
  UefiExtraFdBaseAddress = PcdGet64 (PcdUefiExtraFdBaseAddress);

  Fv = (EFI_FIRMWARE_VOLUME_HEADER *)AllocatePages (
                                       EFI_SIZE_TO_PAGES (UefiExtraFdSize)
                                       );

  if (Fv == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Failed to allocate memory for Fv\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "Read UEFI extra region at 0x%llx Size: "
    "0x%x\n",
    UefiExtraFdBaseAddress,
    UefiExtraFdSize
    ));
  FlashReadCommand (UefiExtraFdBaseAddress, Fv, UefiExtraFdSize);
  PeiServicesInstallFvInfo2Ppi (
    &Fv->FileSystemGuid,
    (VOID *)(UINTN)Fv,
    UefiExtraFdSize,
    NULL,
    NULL,
    FALSE
    );
  BuildFvHob ((EFI_PHYSICAL_ADDRESS)Fv, UefiExtraFdSize);

  return EFI_SUCCESS;
}
