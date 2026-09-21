/** @file
  AMD Platform HOB generator for the PEI phase.

  Copyright (C) 2025 - 2026, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Guid/MmCommBuffer.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Returns the number of MM communication buffer pages from PCD.

  Declared as a weak symbol so host-based unit tests can substitute their own
  implementation to control the value per test case without needing a dynamic
  PCD.  Production firmware always uses the FixedAtBuild PCD value.

  @retval  Page count for the MM communication buffer.
**/
#ifdef __GNUC__
__attribute__ ((weak))
#endif
UINT32
EFIAPI
GetMmCommBufferPageCount (
  VOID
  )
{
  return PcdGet32 (PcdMmCommBufferPages);
}

/**
  Generates a MM_COMM_BUFFER HOB.

  This function creates a HOB of type MM_COMM_BUFFER, which is used for
  communication between the MM environment and the rest of the system.
  The size of the buffer is determined by the PcdMmCommBufferPages PCD.

  @retval EFI_SUCCESS           The HOB was successfully created.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to create the HOB.
**/
EFI_STATUS
EFIAPI
GenerateMmCommBufferHob (
  VOID
  )
{
  MM_COMM_BUFFER  *MmCommBuffer;

  MmCommBuffer = BuildGuidHob (&gMmCommBufferHobGuid, sizeof (MM_COMM_BUFFER));
  if (MmCommBuffer == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  MmCommBuffer->NumberOfPages = GetMmCommBufferPageCount ();
  MmCommBuffer->PhysicalStart = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateReservedPages (MmCommBuffer->NumberOfPages & MAX_UINTN);
  if (MmCommBuffer->PhysicalStart == 0) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "MM_COMM_BUFFER HOB: NumberOfPages = %u, PhysicalStart = 0x%lx\n",
    MmCommBuffer->NumberOfPages,
    MmCommBuffer->PhysicalStart
    ));
  return EFI_SUCCESS;
}

/**
  This function handles PlatformHob generation task task at the end of PEI

  @param[in]  PeiServices       Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor  Pointer to the descriptor for the Notification event that
                                caused this function to execute.
  @param[in]  Ppi               Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS       The function completes successfully
  @retval     others
**/
EFI_STATUS
EFIAPI
PlatformHobEndOfPei (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS  Status;

  if (GetMmCommBufferPageCount () != 0) {
    Status = GenerateMmCommBufferHob ();
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: GenerateMmCommBufferHob failed: %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: MM_COMM_BUFFER HOB generation successful with %d pages.\n",
      __func__,
      GetMmCommBufferPageCount ()
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: PcdMmCommBufferPages is 0, skipping MM_COMM_BUFFER HOB generation.\n",
      __func__
      ));
  }

  DEBUG ((DEBUG_INFO, "%a Exit.\n", __func__));

  return EFI_SUCCESS;
}

static EFI_PEI_NOTIFY_DESCRIPTOR  mEndOfPeiNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  (EFI_PEIM_NOTIFY_ENTRY_POINT)PlatformHobEndOfPei
};

/**
  PEIM entry point.

  @param FileHandle      Handle of the file being invoked.
  @param PeiServices     Pointer to PEI Services Table.

  @retval EFI_SUCCESS    The PEIM executed successfully.
**/
EFI_STATUS
EFIAPI
PlatformHobPeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a Entered.\n", __func__));
  //
  // Performing PlatformHobEndOfPei after EndOfPei PPI produced
  //
  Status = PeiServicesNotifyPpi (&mEndOfPeiNotifyList);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "%a Exit.\n", __func__));
  return Status;
}
