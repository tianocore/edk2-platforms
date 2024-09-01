/**@file
  Library to install platform PPI before PEI Core

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

#include <IndustryStandard/RiscVOpensbi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/PlatformSecPpiLib.h>
#include <Library/PrintLib.h>
#include <Library/RiscVFirmwareContextLib.h>

#include <Ppi/TemporaryRamDone.h>
#include <Ppi/TemporaryRamSupport.h>

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

EFI_STATUS
EFIAPI
TemporaryRamDone (
  VOID
  );

STATIC EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI mTemporaryRamSupportPpi = {
  TemporaryRamMigration
};

STATIC EFI_PEI_TEMPORARY_RAM_DONE_PPI mTemporaryRamDonePpi = {
  TemporaryRamDone
};

STATIC EFI_PEI_PPI_DESCRIPTOR mPrivateDispatchTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamDonePpiGuid,
    &mTemporaryRamDonePpi
  },
};

/** Temporary RAM migration function.

  This function migrates the data from temporary RAM to permanent
  memory.

  @param[in]  PeiServices           PEI service
  @param[in]  TemporaryMemoryBase   Temporary memory base address
  @param[in]  PermanentMemoryBase   Permanent memory base address
  @param[in]  CopySize              Size to copy

**/
EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  VOID      *OldHeap;
  VOID      *NewHeap;
  VOID      *OldStack;
  VOID      *NewStack;
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContext;

  DEBUG ((DEBUG_INFO,
    "%a: Temp Mem Base:0x%Lx, Permanent Mem Base:0x%Lx, CopySize:0x%Lx\n",
    __func__,
    TemporaryMemoryBase,
    PermanentMemoryBase,
    (UINT64)CopySize
    ));

  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + (CopySize >> 1));

  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + (CopySize >> 1));
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  CopyMem (NewHeap, OldHeap, CopySize >> 1);   // Migrate Heap
  CopyMem (NewStack, OldStack, CopySize >> 1); // Migrate Stack

  //
  // Reset firmware context pointer
  //
  GetFirmwareContextPointer (&FirmwareContext);
  FirmwareContext = (VOID *)FirmwareContext + (unsigned long)((UINTN)NewStack - (UINTN)OldStack);
  SetFirmwareContextPointer (FirmwareContext);

  //
  // Relocate PEI Service **
  //
  FirmwareContext->PeiServiceTable += (unsigned long)((UINTN)NewStack - (UINTN)OldStack);
  DEBUG ((DEBUG_INFO, "%a: OpenSBI Firmware Context is relocated to 0x%x\n", __func__, FirmwareContext));
  DEBUG ((DEBUG_INFO, "OpenSBI Firmware Context at 0x%x\n", FirmwareContext));
  DEBUG ((DEBUG_INFO, "             PEI Service at 0x%x\n\n", FirmwareContext->PeiServiceTable));

  register uintptr_t a0 asm ("a0") = (uintptr_t)((UINTN)NewStack - (UINTN)OldStack);
  asm volatile ("add sp, sp, a0"::"r"(a0):);
  return EFI_SUCCESS;
}

/** Temprary RAM done function.

**/
EFI_STATUS EFIAPI TemporaryRamDone (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "%a: 2nd time PEI core, temporary ram done.\n", __func__));
  return EFI_SUCCESS;
}
/** Return platform SEC PPI before PEI Core

  @param[in,out]  ThisPpiList   Pointer to retrieve EFI_PEI_PPI_DESCRIPTOR.

**/
EFI_STATUS
GetPlatformPrePeiCorePpiDescriptor (
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **ThisPpiList
)
{
  *ThisPpiList = mPrivateDispatchTable;
  return EFI_SUCCESS;
}

