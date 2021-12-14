/** @file

  Copyright (c) 2023, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>

#include <MorelloPlatform.h>
#include <Library/FdtLib.h>
#include <Library/BaseMemoryLib.h>

/**
  The entrypoint of the module, parse NtFwConfig and produce the PPI and HOB.

  @param[in]  FileHandle   Handle of the file being invoked.
  @param[in]  PeiServices  Describes the list of possible PEI Services.

  @retval EFI_SUCCESS      Either no HW_CONFIG was given by EL3 firmware
                           OR the Morello FDT HOB was successfully created.
  @retval EFI_UNSUPPORTED  FDT header sanity check failed.
  @retval *                Other errors are possible.
**/
EFI_STATUS
EFIAPI
Load (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  STATIC EFI_PEI_PPI_DESCRIPTOR           gPpi;
  CONST MORELLO_EL3_FW_HANDOFF_PARAM_PPI  *ParamPpi;
  CONST UINT64                            *DdrProperty;
  EFI_STATUS                              Status;
  INT32                                   Offset;
  MORELLO_PLAT_INFO_FVP                   *PlatInfo;

  PlatInfo = BuildGuidHob (
               &gArmMorelloPlatformInfoDescriptorGuid,
               sizeof (*PlatInfo)
               );

  if (PlatInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to allocate platform info HOB\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiServicesLocatePpi (
             &gArmMorelloParameterPpiGuid,
             0,
             NULL,
             (VOID **)&ParamPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to locate gArmMorelloParameterPpiGuid - %r\n",
      gEfiCallerBaseName,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (FdtCheckHeader (ParamPpi->NtFwConfig) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB\n",
      ParamPpi->NtFwConfig,
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  Offset = FdtSubnodeOffset (ParamPpi->NtFwConfig, 0, "platform-info");
  if (Offset == -FDT_ERR_NOTFOUND) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: platform-info node not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  DdrProperty = FdtGetProp (
                  ParamPpi->NtFwConfig,
                  Offset,
                  "local-ddr-size",
                  NULL
                  );
  if (DdrProperty == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: local-ddr-size property not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  PlatInfo->LocalDdrSize = Fdt64ToCpu (ReadUnaligned64 (DdrProperty));

  gPpi.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI
               | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  gPpi.Guid = &gArmMorelloFvpPlatformInfoDescriptorPpiGuid;
  gPpi.Ppi  = PlatInfo;

  Status = PeiServicesInstallPpi (&gPpi);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to install PEI service - %r\n",
      gEfiCallerBaseName,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
