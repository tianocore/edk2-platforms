/** @file
  Qualcomm platform library

  Copyright (c) 2011-2012, ARM Limited. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>

#include <Ppi/ArmMpCoreInfo.h>

ARM_CORE_INFO  mArmPlatformNullMpCoreInfoTable[] = {
  {
    // Cluster 0, Core 0
    0x0,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)0,
    (EFI_PHYSICAL_ADDRESS)0,
    (EFI_PHYSICAL_ADDRESS)0,
    (UINT64)0xFFFFFFFF
  }
};

/**
  Return the current Boot Mode.

  This function returns the boot reason on the platform.

  @retval EFI_BOOT_MODE  The current boot mode.

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  Initialize controllers that must setup in the normal world.

  This function is called by the ArmPlatformPkg/PrePi or ArmPlatformPkg/PlatformPei
  in the PEI phase.

  @param[in]  MpId  The MPIDR of the current processor.

  @retval RETURN_SUCCESS  Platform initialization completed successfully.

**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN  MpId
  )
{
  return RETURN_SUCCESS;
}

/**
  Get the Multi-Processor Core Information for Pre-PEI Core.

  This function provides core information during the Pre-PEI phase.

  @param[out]  CoreCount     Pointer to store the number of cores.
  @param[out]  ArmCoreTable  Pointer to store the ARM core table.

  @retval EFI_SUCCESS      Core information retrieved successfully.
  @retval EFI_UNSUPPORTED  Multi-core is not supported.

**/
EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN          *CoreCount,
  OUT ARM_CORE_INFO  **ArmCoreTable
  )
{
  if (ArmIsMpCore ()) {
    *CoreCount    = sizeof (mArmPlatformNullMpCoreInfoTable) / sizeof (ARM_CORE_INFO);
    *ArmCoreTable = mArmPlatformNullMpCoreInfoTable;
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

ARM_MP_CORE_INFO_PPI  mMpCoreInfoPpi = { PrePeiCoreGetMpCoreInfo };

EFI_PEI_PPI_DESCRIPTOR  gPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

/**
  Get the Platform PPI List.

  This function returns the list of Platform PPIs.

  @param[out]  PpiListSize  Pointer to store the size of the PPI list.
  @param[out]  PpiList      Pointer to store the PPI list.

**/
VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  if (ArmIsMpCore ()) {
    *PpiListSize = sizeof (gPlatformPpiTable);
    *PpiList     = gPlatformPpiTable;
  } else {
    *PpiListSize = 0;
    *PpiList     = NULL;
  }
}
