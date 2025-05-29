/** @file

  Copyright (c) 2024, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>

#include <NeoverseN1Soc.h>

STATIC EFI_PEI_PPI_DESCRIPTOR  mPpi;

/**
  The entrypoint of the module, parse NtFwConfig and produce the PPI and HOB.

  @param[in]  FileHandle       Handle of the file being invoked.
  @param[in]  PeiServices      Describes the list of possible PEI Services.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
  @retval EFI_NOT_FOUND         Error processing the DTB.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate memory for the HOB.
**/
EFI_STATUS
EFIAPI
NtFwConfigPeEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                                    Status;
  CONST NEOVERSEN1SOC_EL3_FW_HANDOFF_PARAM_PPI  *ParamPpi;
  CONST UINT32                                  *Property;
  INT32                                         Offset;
  NEOVERSEN1SOC_PLAT_INFO                       *PlatInfo;
  CONST VOID                                    *FdtBase;

  Status = PeiServicesLocatePpi (
             &gArmNeoverseN1SocParameterPpiGuid,
             0,
             NULL,
             (VOID **)&ParamPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to locate gArmNeoverseN1SocParameterPpiGuid - %r\n",
      gEfiCallerBaseName,
      Status
      ));
    return Status;
  }

  if ((ParamPpi == NULL) || (ParamPpi->NtFwConfig == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FdtBase = ParamPpi->NtFwConfig;
  if (FdtCheckHeader (FdtBase) != 0) {
    DEBUG ((DEBUG_ERROR, "Invalid DTB file %p passed\n", FdtBase));
    return EFI_INVALID_PARAMETER;
  }

  PlatInfo = BuildGuidHob (
               &gArmNeoverseN1SocPlatformInfoDescriptorGuid,
               sizeof (*PlatInfo)
               );
  if (PlatInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to allocate platform info HOB\n",
      gEfiCallerBaseName
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  Offset = FdtSubnodeOffset (FdtBase, 0, "platform-info");
  if (Offset == -FDT_ERR_NOTFOUND) {
    DEBUG ((DEBUG_ERROR, "Invalid DTB : platform-info node not found\n"));
    return EFI_NOT_FOUND;
  }

  Property = FdtGetProp (FdtBase, Offset, "local-ddr-size", NULL);
  if (Property == NULL) {
    DEBUG ((DEBUG_ERROR, "local-ddr-size property not found\n"));
    return EFI_NOT_FOUND;
  }

  PlatInfo->LocalDdrSize = Fdt32ToCpu (*Property);

  Property = FdtGetProp (FdtBase, Offset, "remote-ddr-size", NULL);
  if (Property == NULL) {
    DEBUG ((DEBUG_ERROR, "remote-ddr-size property not found\n"));
    return EFI_NOT_FOUND;
  }

  PlatInfo->RemoteDdrSize = Fdt32ToCpu (*Property);

  Property = FdtGetProp (FdtBase, Offset, "secondary-chip-count", NULL);
  if (Property == NULL) {
    DEBUG ((DEBUG_ERROR, "secondary-chip-count property not found\n"));
    return EFI_NOT_FOUND;
  }

  PlatInfo->SecondaryChipCount = Fdt32ToCpu (*Property);

  Property = FdtGetProp (FdtBase, Offset, "multichip-mode", NULL);
  if (Property == NULL) {
    DEBUG ((DEBUG_ERROR, "multichip-mode property not found\n"));
    return EFI_NOT_FOUND;
  }

  PlatInfo->MultichipMode = Fdt32ToCpu (*Property);

  mPpi.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI
               | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  mPpi.Guid = &gArmNeoverseN1SocPlatformInfoDescriptorPpiGuid;
  mPpi.Ppi  = PlatInfo;

  Status = PeiServicesInstallPpi (&mPpi);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to install PEI service - %r\n",
      gEfiCallerBaseName,
      Status
      ));
  }

  return Status;
}
