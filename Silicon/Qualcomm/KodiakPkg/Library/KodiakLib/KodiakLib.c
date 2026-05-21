/** @file

  Kodiak platform support functions.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>

#include <Ppi/ArmMpCoreInfo.h>

ARM_CORE_INFO  mPlatformCoreInfoTable[] = {
  {
    // Cluster 0, Core 0
    0x000,
  },
};

/**
  Timer constructor.

  This function should be better located into TimerLib implementation.

  @retval EFI_SUCCESS   The timer was initialized successfully.
**/
EFI_STATUS
EFIAPI
TimerConstructor (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Return the current Boot Mode

  This function returns the boot mode on the platform.

  @retval BOOT_WITH_FULL_CONFIGURATION  Perform a full configuration boot.
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

  @param[in] MpId  The Multiprocessor Affinity Register (MPIDR) value of the core.

  @retval EFI_SUCCESS  Initialization completed successfully.
**/
EFI_STATUS
ArmPlatformInitialize (
  IN  UINTN  MpId
  )
{
  return EFI_SUCCESS;
}

/**
  Retrieves the multi-processor core information table for the platform.

  @param[out] CoreCount     Pointer to receive the number of entries in the core info table.
  @param[out] ArmCoreTable  Pointer to receive the base address of the ARM core info table.

  @retval EFI_SUCCESS       The core information was successfully retrieved.
  @retval EFI_UNSUPPORTED   The platform does not support MP Core information.
**/
EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN          *CoreCount,
  OUT ARM_CORE_INFO  **ArmCoreTable
  )
{
  if (ArmIsMpCore ()) {
    *CoreCount    = ARRAY_SIZE (mPlatformCoreInfoTable);
    *ArmCoreTable = mPlatformCoreInfoTable;
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
  Retrieves the platform-specific PPI list.

  @param[out] PpiListSize  Size in bytes of the PPI list.
  @param[out] PpiList      Pointer to the platform PPI descriptor table.
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
