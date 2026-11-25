/** @file
  ArmPlatformLib implementation for Allwinner H618 SoC.

  Copyright (c) 2026, Khai Do <dovankhai97@gmail.com>. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <AllwinnerH618.h>

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  6

/**
  Return the current Boot Mode.

  @return  Boot Mode of the platform.
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

  @param[in]  MpId  Core processor ID.

  @retval  RETURN_SUCCESS  Platform successfully initialized.
**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN  MpId
  )
{
  return RETURN_SUCCESS;
}

/**
  Return the Virtual Memory Map of your platform.

  @param[out]  VirtualMemoryMap  Array of ARM_MEMORY_REGION_DESCRIPTOR.
**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINTN                         Index;

  ASSERT (VirtualMemoryMap != NULL);
  if (VirtualMemoryMap == NULL) {
    return;
  }

  VirtualMemoryTable = AllocatePool (
                         sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );
  if (VirtualMemoryTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate VirtualMemoryTable\n", __func__));
    *VirtualMemoryMap = NULL;
    return;
  }

  Index = 0;

  //
  // System SDRAM (DDR)
  //
  VirtualMemoryTable[Index].PhysicalBase = FixedPcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].VirtualBase  = FixedPcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].Length       = FixedPcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  Index++;

  //
  // SRAM A1 + SRAM C
  //
  VirtualMemoryTable[Index].PhysicalBase = AW_H618_SRAM_A1_BASE;
  VirtualMemoryTable[Index].VirtualBase  = AW_H618_SRAM_A1_BASE;
  VirtualMemoryTable[Index].Length       = AW_H618_SRAM_A1_SIZE + AW_H618_SRAM_C_SIZE;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
  Index++;

  //
  // SRAM A2
  //
  VirtualMemoryTable[Index].PhysicalBase = AW_H618_SRAM_A2_BASE;
  VirtualMemoryTable[Index].VirtualBase  = AW_H618_SRAM_A2_BASE;
  VirtualMemoryTable[Index].Length       = AW_H618_SRAM_A2_SIZE;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
  Index++;

  //
  // SoC Low Peripherals (0x03000000 - 0x03FFFFFF)
  //
  VirtualMemoryTable[Index].PhysicalBase = 0x03000000ULL;
  VirtualMemoryTable[Index].VirtualBase  = 0x03000000ULL;
  VirtualMemoryTable[Index].Length       = 0x01000000ULL;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  Index++;

  //
  // SoC High Peripherals (0x04000000 - 0x09FFFFFF)
  //
  VirtualMemoryTable[Index].PhysicalBase = 0x04000000ULL;
  VirtualMemoryTable[Index].VirtualBase  = 0x04000000ULL;
  VirtualMemoryTable[Index].Length       = 0x06000000ULL;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  Index++;

  //
  // End of Table
  //
  VirtualMemoryTable[Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase  = 0;
  VirtualMemoryTable[Index].Length       = 0;
  VirtualMemoryTable[Index].Attributes   = 0;
  Index++;

  ASSERT (Index <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}

STATIC ARM_CORE_INFO  mH618CoreInfoTable[] = {
  { 0x0 }, // Cluster 0, Core 0
  { 0x1 }, // Cluster 0, Core 1
  { 0x2 }, // Cluster 0, Core 2
  { 0x3 }, // Cluster 0, Core 3
};

/**
  Return the multi-core info table for the platform.

  @param[out]  CoreCount     Number of cores in the platform.
  @param[out]  ArmCoreTable  Array of ARM_CORE_INFO structures.

  @retval  EFI_SUCCESS            Table returned successfully.
  @retval  EFI_INVALID_PARAMETER  CoreCount or ArmCoreTable is NULL.
**/
STATIC
EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN          *CoreCount,
  OUT ARM_CORE_INFO  **ArmCoreTable
  )
{
  if ((CoreCount == NULL) || (ArmCoreTable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *CoreCount    = sizeof (mH618CoreInfoTable) / sizeof (ARM_CORE_INFO);
  *ArmCoreTable = mH618CoreInfoTable;

  return EFI_SUCCESS;
}

STATIC ARM_MP_CORE_INFO_PPI  mMpCoreInfoPpi = {
  PrePeiCoreGetMpCoreInfo
};

STATIC EFI_PEI_PPI_DESCRIPTOR  mPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

/**
  Return the Platform specific PPIs.

  @param[out]  PpiListSize  Size in Bytes of the Platform PPI List.
  @param[out]  PpiList      Platform PPI List.
**/
VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  if (PpiListSize != NULL) {
    *PpiListSize = sizeof (mPlatformPpiTable);
  }

  if (PpiList != NULL) {
    *PpiList = mPlatformPpiTable;
  }
}
