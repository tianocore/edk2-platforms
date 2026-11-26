/** @file

  ArmPlatformLib implementation for the CN10K line of SoCs.

  Copyright (c) 2015-2015, Cavium Inc. All rights reserved.<BR>
  Copyright (c) 2011-2013, ARM Limited. All rights reserved.<BR>
  Copyright (c) 2026, Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/IoLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#include <Ppi/ArmMpCoreInfo.h>

UINT64  mDeviceTreeBaseAddress = 0;

STATIC ARM_CORE_INFO  mCn10kMpCoreInfo[FixedPcdGet32 (PcdMaxCoresPerNode)];

EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN  MpId
  )
{
  return RETURN_SUCCESS;
}

VOID
ArmPlatformInitializeSystemMemory (
  VOID
  )
{
  // Nothing to do here
}

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN          *CoreCount,
  OUT ARM_CORE_INFO  **ArmCoreTable
  )
{
  UINTN  Index;

  if (!ArmIsMpCore ()) {
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < ARRAY_SIZE (mCn10kMpCoreInfo); Index++) {
    mCn10kMpCoreInfo[Index].Mpidr = LShiftU64 (Index, 16);
  }

  *CoreCount    = ARRAY_SIZE (mCn10kMpCoreInfo);
  *ArmCoreTable = mCn10kMpCoreInfo;

  return EFI_SUCCESS;
}

ARM_MP_CORE_INFO_PPI  mMpCoreInfoPpi = { PrePeiCoreGetMpCoreInfo };

EFI_PEI_PPI_DESCRIPTOR  gPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = sizeof (gPlatformPpiTable);
  *PpiList     = gPlatformPpiTable;
}

EFI_STATUS
EFIAPI
CN10KConstructor (
  VOID
  )
{
  if (!BuildGuidDataHob (&gFdtHobGuid, &mDeviceTreeBaseAddress, sizeof (VOID *))) {
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}
