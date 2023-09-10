/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

EFI_STATUS
EFIAPI
BootServicesInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;
  UINTN                  MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap, *Desc;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  EFI_PHYSICAL_ADDRESS   Addr;
  UINTN                  Idx;
  UINTN                  Pages;

  MemoryMap     = NULL;
  MemoryMapSize = 0;
  Pages         = 0;

  Status = gBS->GetMemoryMap (
                              &MemoryMapSize,
                              MemoryMap,
                              &MapKey,
                              &DescriptorSize,
                              &DescriptorVersion
                              );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    Pages     = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
    MemoryMap = AllocatePages (Pages);

    //
    // Get System MemoryMap
    //
    Status = gBS->GetMemoryMap (
                                &MemoryMapSize,
                                MemoryMap,
                                &MapKey,
                                &DescriptorSize,
                                &DescriptorVersion
                                );
  }

  if (EFI_ERROR (Status) || (MemoryMap == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get memory map\n", __func__));
    if (MemoryMap) {
      FreePages ((VOID *)MemoryMap, Pages);
    }

    return EFI_SUCCESS;
  }

  Desc = MemoryMap;

  //
  // Change 64-bit EfiConventionalMemory with EfiBootServicesData
  // So that DXE driver only allocate EfiBootServicesData in 32-bit addresses
  //
  for (Idx = 0; Idx < (MemoryMapSize / DescriptorSize); Idx++) {
    if ((Desc->Type == EfiConventionalMemory) &&
        ((Desc->PhysicalStart > MAX_UINT32) || ((Desc->PhysicalStart + Desc->NumberOfPages * EFI_PAGE_SIZE) > MAX_UINT32)))
    {
      if (Desc->PhysicalStart > MAX_UINT32) {
        Addr   = Desc->PhysicalStart;
        Status = gBS->AllocatePages (
                                     AllocateAddress,
                                     EfiBootServicesData,
                                     Desc->NumberOfPages,
                                     &Addr
                                     );
      } else {
        Addr   = 0x100000000ULL;
        Status = gBS->AllocatePages (
                                     AllocateAddress,
                                     EfiBootServicesData,
                                     Desc->NumberOfPages - EFI_SIZE_TO_PAGES (0x100000000 - Desc->PhysicalStart),
                                     &Addr
                                     );
      }

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to allocate boot service data at %llX\n", __func__, Addr));
      }
    }

    Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT64)Desc + DescriptorSize);
  }

  FreePages ((VOID *)MemoryMap, Pages);

  return EFI_SUCCESS;
}
