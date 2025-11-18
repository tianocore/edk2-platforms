/** @file

  Copyright (c) 2023, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>

#include <Guid/FdtHob.h>

#include <MorelloPlatform.h>

/**
  The entrypoint of the module, it will pass the FDT via a HOB.

  @param[in]  FileHandle   Handle of the file being invoked.
  @param[in]  PeiServices  Describes the list of possible PEI Services.

  @retval EFI_SUCCESS      Either no HW_CONFIG was given by EL3 firmware
                           OR the Morello FDT HOB was successfully created.
  @retval EFI_UNSUPPORTED  FDT header sanity check failed.
  @retval *                Other errors are possible.
**/
EFI_STATUS
EFIAPI
MorelloHwConfigEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  CONST MORELLO_EL3_FW_HANDOFF_PARAM_PPI  *ParamPpi;
  EFI_STATUS                              Status;
  UINT64                                  *FdtHobData;
  UINTN                                   FdtPages;
  UINTN                                   FdtSize;
  VOID                                    *FdtNewBase;

  Status = PeiServicesLocatePpi (
             &gArmMorelloParameterPpiGuid,
             0,
             NULL,
             (VOID **)&ParamPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: PeiServicesLocatePpi() failure -- %r\n",
      gEfiCallerBaseName,
      Status
      ));
    // this is really not expected
    ASSERT (FALSE);
    return Status;
  }

  if (ParamPpi->HwConfig == NULL) {
    DEBUG ((DEBUG_INFO, "No HW_CONFIG\n"));
    return EFI_SUCCESS;
  }

  if (FdtCheckHeader (ParamPpi->HwConfig) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid FDT in HW_CONFIG\n",
      gEfiCallerBaseName
      ));
    return EFI_UNSUPPORTED;
  }

  FdtSize    = FdtTotalSize (ParamPpi->HwConfig);
  FdtPages   = EFI_SIZE_TO_PAGES (FdtSize);
  FdtNewBase = AllocatePages (FdtPages);
  if (FdtNewBase == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: unable to allocate heap storage for HW_CONFIG\n",
      gEfiCallerBaseName
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  FdtOpenInto (ParamPpi->HwConfig, FdtNewBase, EFI_PAGES_TO_SIZE (FdtPages));

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof (*FdtHobData));
  if (FdtHobData == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: unable to allocate FDT HOB!\n",
      gEfiCallerBaseName
      ));
    FreePool (FdtNewBase);
    return EFI_OUT_OF_RESOURCES;
  }

  *FdtHobData = (UINTN)FdtNewBase;

  return EFI_SUCCESS;
}
