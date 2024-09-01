/** @file
  Sample to provide FSP wrapper related function.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <FspEas/FspApi.h>
#include <FspmUpd.h>
#include <FspsUpd.h>
#include <Library/HobLib.h>
#include <Guid/SmramMemoryReserve.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/Smbus2.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/PeiServicesLib.h>
#include <Guid/AcpiS3Context.h>

#pragma pack(push, 1)
typedef struct {
  UINT8    connector_type;
  UINT8    aux_index;
  UINT8    hdp_index;
  UINT8    reserved;
} fsp_ddi_descriptor;
#pragma pack(pop)

extern EFI_GUID  gEfiSmmPeiSmramMemoryReserveGuid;

extern EFI_GUID  gAmdResourceSizeForEachRbGuid;
extern EFI_GUID  gAmdTotalNumberOfRootBridgesGuid;
extern EFI_GUID  gAmdPbsSystemConfigurationGuid;
extern EFI_GUID  gApSyncFlagNvVariableGuid;

typedef enum {
  IDS_HOOK_SUCCESS = 0,         ///< The service completed normally
  IDS_HOOK_UNSUPPORTED,         ///< Unsupported IDS HOOK
  IDS_HOOK_BUFFER_TOO_SMALL,    ///< Too small buffer
  IDS_HOOK_NOT_FOUND,           ///< Haven't found accordingly service entry for specific IDS HOOK ID
  IDS_HOOK_ERROR,               ///< Error happens during service IDS HOOK
  IDS_HOOK_SKIP,                ///< Use to notify the IDS HOOK caller to skip a block of codes, used for IDS_HOOK_SKIP
  IDS_HOOK_NO_SKIP,             ///< Use to notify the IDS HOOK caller not skip a block of codes, used for IDS_HOOK_SKIP
  IDS_HOOK_MAX                  ///< Not a status, for limit checking.
} IDS_HOOK_STATUS;

IDS_HOOK_STATUS
GetIdsNvTable (
  IN OUT   VOID    *IdsNvTable,
  IN OUT   UINT32  *IdsNvTableSize
  );

STATIC
EFI_STATUS
GetIdsNvData (
  FSPM_UPD *volatile  FspmUpd
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function overrides the default configurations in the FSP-S UPD data region.

  @param[in,out] FspUpdRgnPtr   A pointer to the UPD data region data structure.

**/
VOID
EFIAPI
UpdateFspsUpdData (
  IN OUT VOID  *FspUpdRgnPtr
  )
{
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramHob = GetFirstGuidHob (
                                                &gEfiSmmPeiSmramMemoryReserveGuid
                                                );

  if (SmramHob != NULL) {
    ((FSPS_UPD *)FspUpdRgnPtr)->FspsConfig.smram_hob_base_addr = (UINT32)(UINTN)GET_GUID_HOB_DATA (SmramHob);
    ((FSPS_UPD *)FspUpdRgnPtr)->FspsConfig.smram_hob_size      =      (UINT32)GET_GUID_HOB_DATA_SIZE (SmramHob);
  }

  EFI_SMRAM_DESCRIPTOR  *SmramDescriptor = GetFirstGuidHob (&gEfiAcpiVariableGuid);

  if (SmramDescriptor != NULL) {
    ((FSPS_UPD *)FspUpdRgnPtr)->FspsConfig.smram_hob_descriptor_base_addr = (UINT32)(UINTN)GET_GUID_HOB_DATA (SmramDescriptor);
    ((FSPS_UPD *)FspUpdRgnPtr)->FspsConfig.smram_hob_descriptor_size      =      (UINT32)GET_GUID_HOB_DATA_SIZE (SmramDescriptor);
  } else {
    DEBUG ((DEBUG_ERROR, "Cannot found SmramDescriptor!\n"));
  }

  ((FSPS_UPD *)FspUpdRgnPtr)->FspsConfig.fsp_o_dxe_volume_address = PcdGet32 (PcdFspoDxeBaseAddressInMemory);
  ((FSPS_UPD *)FspUpdRgnPtr)->FspsConfig.page_address_below_1mb   = 0x10000;
}

/**
  This function overrides the default configurations in the FSP-M UPD data region.

  @note At this point, memory is NOT ready, PeiServices are available to use.

  @param[in,out] FspUpdRgnPtr   A pointer to the UPD data region data structure.

**/
VOID
EFIAPI
UpdateFspmUpdDataForFabric (
  IN OUT VOID  *FspUpdRgnPtr
  )
{
  DEBUG ((DEBUG_INFO, "%a Enter\n", __func__));
  FSPM_UPD                         *Upd           = (FSPM_UPD *)FspUpdRgnPtr;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *ReadVariable2 = NULL;
  EFI_STATUS                       Status         = PeiServicesLocatePpi (&gEfiPeiReadOnlyVariable2PpiGuid, 0, NULL, (VOID **)&ReadVariable2);

  ASSERT (Status == EFI_SUCCESS);
  UINT32  VariableSize = 0;
  VOID    *Buffer      = NULL;

  Status = ReadVariable2->GetVariable (
                            ReadVariable2,
                            L"ResourceSizeForEachRb",
                            &gAmdResourceSizeForEachRbGuid,
                            NULL,
                            &VariableSize,
                            NULL
                            );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocatePool (VariableSize);
    ASSERT (Buffer != NULL);
    Status = ReadVariable2->GetVariable (
                              ReadVariable2,
                              L"ResourceSizeForEachRb",
                              &gAmdResourceSizeForEachRbGuid,
                              NULL,
                              &VariableSize,
                              Buffer
                              );
    if (!EFI_ERROR (Status)) {
      Upd->FspmConfig.resource_size_for_each_rb_ptr  = (UINT32)(UINTN)Buffer;
      Upd->FspmConfig.resource_size_for_each_rb_size = VariableSize;
    }
  }

  DEBUG ((DEBUG_INFO, "Get variable %s returns %r\n", L"ResourceSizeForEachRb", Status));
  VariableSize = 0;
  Buffer       = NULL;
  Status       = ReadVariable2->GetVariable (
                                  ReadVariable2,
                                  L"TotalNumberOfRootBridges",
                                  &gAmdTotalNumberOfRootBridgesGuid,
                                  NULL,
                                  &VariableSize,
                                  NULL
                                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocatePool (VariableSize);
    ASSERT (Buffer != NULL);
    Status = ReadVariable2->GetVariable (
                              ReadVariable2,
                              L"TotalNumberOfRootBridges",
                              &gAmdTotalNumberOfRootBridgesGuid,
                              NULL,
                              &VariableSize,
                              Buffer
                              );
    if (!EFI_ERROR (Status)) {
      Upd->FspmConfig.total_number_of_root_bridges_ptr  = (UINT32)(UINTN)Buffer;
      Upd->FspmConfig.total_number_of_root_bridges_size = VariableSize;
    }
  }

  DEBUG ((DEBUG_INFO, "Get variable %s returns %r\n", L"TotalNumberOfRootBridges", Status));
  VariableSize = 0;
  Buffer       = NULL;
  Status       = ReadVariable2->GetVariable (
                                  ReadVariable2,
                                  L"AMD_PBS_SETUP",
                                  &gAmdPbsSystemConfigurationGuid,
                                  NULL,
                                  &VariableSize,
                                  NULL
                                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocatePool (VariableSize);
    ASSERT (Buffer != NULL);
    Status = ReadVariable2->GetVariable (
                              ReadVariable2,
                              L"AMD_PBS_SETUP",
                              &gAmdPbsSystemConfigurationGuid,
                              NULL,
                              &VariableSize,
                              Buffer
                              );
    if (!EFI_ERROR (Status)) {
      Upd->FspmConfig.amd_pbs_setup_ptr  = (UINT32)(UINTN)Buffer;
      Upd->FspmConfig.amd_pbs_setup_size = VariableSize;
    }
  }

  DEBUG ((DEBUG_INFO, "Get variable %s returns %r\n", L"AMD_PBS_SETUP", Status));
  VariableSize = 0;
  Buffer       = NULL;
  Status       = ReadVariable2->GetVariable (
                                  ReadVariable2,
                                  L"ApSyncFlagNv",
                                  &gApSyncFlagNvVariableGuid,
                                  NULL,
                                  &VariableSize,
                                  NULL
                                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocatePool (VariableSize);
    ASSERT (Buffer != NULL);
    Status = ReadVariable2->GetVariable (
                              ReadVariable2,
                              L"ApSyncFlagNv",
                              &gApSyncFlagNvVariableGuid,
                              NULL,
                              &VariableSize,
                              Buffer
                              );
    if (!EFI_ERROR (Status)) {
      Upd->FspmConfig.ap_sync_flag_nv_ptr  = (UINT32)(UINTN)Buffer;
      Upd->FspmConfig.ap_sync_flag_nv_size = VariableSize;
    }

    DEBUG ((DEBUG_INFO, "Get variable %s returns %r\n", L"ApSyncFlagNv", Status));
  }
}

/**
  This function overrides the default configurations in the FSP-M UPD data region.

  @note At this point, memory is NOT ready, PeiServices are available to use.

  @param[in,out] FspUpdRgnPtr   A pointer to the UPD data region data structure.

**/
VOID
EFIAPI
UpdateFspmUpdData (
  IN OUT VOID  *FspUpdRgnPtr
  )
{
  FSPM_UPD  *FspmUpd;

  FspmUpd = (FSPM_UPD *)FspUpdRgnPtr;
  EFI_BOOT_MODE  BootMode = BOOT_WITH_FULL_CONFIGURATION;

  PeiServicesGetBootMode (&BootMode);
  FspmUpd->FspmArchUpd.BootMode  = BootMode;
  FspmUpd->FspmArchUpd.StackBase = (VOID *)0x11000; // 1 Page for CPU reset in DXE.
  FspmUpd->FspmArchUpd.StackSize = 0x20000;
  DEBUG ((DEBUG_INFO, "Getting IDS NV Table returns status %r\n", GetIdsNvData (FspmUpd)));
  UpdateFspmUpdDataForFabric (FspUpdRgnPtr);
}

/**
  Update TempRamExit parameter.

  @note At this point, memory is ready, PeiServices are available to use.

  @return TempRamExit parameter.
**/
VOID *
EFIAPI
UpdateTempRamExitParam (
  VOID
  )
{
  return NULL;
}

/**
  Get S3 PEI memory information.

  @note At this point, memory is ready, and PeiServices are available to use.
  Platform can get some data from SMRAM directly.

  @param[out] S3PeiMemSize  PEI memory size to be installed in S3 phase.
  @param[out] S3PeiMemBase  PEI memory base to be installed in S3 phase.

  @return If S3 PEI memory information is got successfully.
**/
EFI_STATUS
EFIAPI
GetS3MemoryInfo (
  OUT UINT64                *S3PeiMemSize,
  OUT EFI_PHYSICAL_ADDRESS  *S3PeiMemBase
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Perform platform related reset in FSP wrapper.

  This function will reset the system with requested ResetType.

  @param[in] FspStatusResetType  The type of reset the platform has to perform.
**/
VOID
EFIAPI
CallFspWrapperResetSystem (
  IN EFI_STATUS  FspStatusResetType
  )
{
  //
  // Perform reset according to the type.
  //

  CpuDeadLoop ();
}
