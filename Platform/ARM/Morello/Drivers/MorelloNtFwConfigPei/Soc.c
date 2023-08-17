/** @file

  Copyright (c) 2023, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>

#include <MorelloPlatform.h>
#include <Library/FdtLib.h>

STATIC EFI_PEI_PPI_DESCRIPTOR  gPpi;

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
  CONST MORELLO_EL3_FW_HANDOFF_PARAM_PPI  *ParamPpi;
  CONST UINT32                            *Property;
  CONST UINT64                            *DdrProperty;
  EFI_STATUS                              Status;
  INT32                                   OffsetPlat;
  INT32                                   OffsetFw;
  MORELLO_PLAT_INFO_SOC                   *PlatInfo;
  MORELLO_FW_VERSION_SOC                  *FwVersion;

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

  FwVersion = BuildGuidHob (
                &gArmMorelloFirmwareVersionGuid,
                sizeof (*FwVersion)
                );

  if (FwVersion == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to allocate firmware version HOB\n",
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

  OffsetPlat = FdtSubnodeOffset (ParamPpi->NtFwConfig, 0, "platform-info");
  if (OffsetPlat == -FDT_ERR_NOTFOUND) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: platform-info node not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  OffsetFw = FdtSubnodeOffset (ParamPpi->NtFwConfig, 0, "firmware-version");
  if (OffsetFw == -FDT_ERR_NOTFOUND) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: firmware-version node not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  DdrProperty = FdtGetProp (
                  ParamPpi->NtFwConfig,
                  OffsetPlat,
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

  DdrProperty = FdtGetProp (
                  ParamPpi->NtFwConfig,
                  OffsetPlat,
                  "remote-ddr-size",
                  NULL
                  );
  if (DdrProperty == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: remote-ddr-size property not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  PlatInfo->RemoteDdrSize = Fdt64ToCpu (ReadUnaligned64 (DdrProperty));

  Property = FdtGetProp (
               ParamPpi->NtFwConfig,
               OffsetPlat,
               "remote-chip-count",
               NULL
               );
  if (Property == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: remote-chip-count property not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  PlatInfo->RemoteChipCount = Fdt32ToCpu (*Property);

  Property = FdtGetProp (ParamPpi->NtFwConfig, OffsetPlat, "multichip-mode", NULL);
  if (Property == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: multichip-mode property not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  PlatInfo->Mode = Fdt32ToCpu (*Property);

  Property = FdtGetProp (ParamPpi->NtFwConfig, OffsetPlat, "scc-config", NULL);
  if (Property == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: scc-config property not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  PlatInfo->SccConfig = Fdt32ToCpu (*Property);

  Property = FdtGetProp (ParamPpi->NtFwConfig, OffsetFw, "scp-fw-version", NULL);
  if (Property == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: scp-fw-version property not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  FwVersion->ScpFwRevision = Fdt32ToCpu (*Property);

  Property = FdtGetProp (ParamPpi->NtFwConfig, OffsetFw, "scp-fw-commit", NULL);
  if (Property == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: invalid NT_FW_CONFIG DTB: scp-fw-commit property not found\n",
      gEfiCallerBaseName
      ));
    return EFI_INVALID_PARAMETER;
  }

  FwVersion->ScpFwCommit = Fdt32ToCpu (*Property);

  gPpi.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI
               | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  gPpi.Guid = &gArmMorelloSocPlatformInfoDescriptorPpiGuid;
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
