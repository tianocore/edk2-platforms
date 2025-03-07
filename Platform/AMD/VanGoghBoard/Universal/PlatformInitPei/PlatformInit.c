/** @file
  Implements Platforminit.C
  This PEIM initialize platform for MRC, following action is performed,
    1. Initizluize GMCH
    2. Detect boot mode
    3. Detect video adapter to determine whether we need pre allocated memory
    4. Calls MRC to initialize memory and install a PPI notify to do post memory initialization.
       This file contains the main entrypoint of the PEIM.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2016，Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"
#define PEI_STALL_RESOLUTION  1

EFI_PEI_STALL_PPI  mStallPpi = {
  PEI_STALL_RESOLUTION,
  Stall
};

EFI_PEI_PPI_DESCRIPTOR  mPpiStall[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiStallPpiGuid,
    &mStallPpi
  }
};

EFI_PEI_PPI_DESCRIPTOR  mPpiBootMode[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};

EFI_PEI_NOTIFY_DESCRIPTOR  mMemoryDiscoveredNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gAmdMemoryInfoHobPpiGuid,
    MemoryInfoHobPpiNotifyCallback
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemoryDiscoveredPpiNotifyCallback
  }
};

/**
 Clear all SMI enable bit in SmiControl0-SmiControl9 register

 @param [in]        None

 @retval            None
*/
VOID
ClearAllSmiControlRegisters (
  VOID
  )
{
  UINTN  SmiControlOffset;

  for (SmiControlOffset = FCH_SMI_REGA0; SmiControlOffset <= FCH_SMI_REGC4; SmiControlOffset += 4) {
    MmioWrite32 (ACPI_MMIO_BASE + SMI_BASE + SmiControlOffset, 0x00);
  }
}

/**
  Clear any SMI status or wake status left over from boot.

  @param  none

  @retval none
**/
VOID
EFIAPI
ClearSmiAndWake (
  VOID
  )
{
  UINT16  Pm1Status;
  UINT16  PmControl;
  UINT16  AcpiBaseAddr;

  AcpiBaseAddr = MmioRead16 (ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REG60);

  //
  // Read the ACPI registers
  //
  Pm1Status = IoRead16 (AcpiBaseAddr);
  PmControl = IoRead16 ((UINT16)(AcpiBaseAddr + R_FCH_ACPI_PM_CONTROL));

  //
  // Clear any SMI or wake state from the boot
  //
  Pm1Status |= 0xFF;          // clear all events
  PmControl &= 0xFFFE;        // clear Bit0(SciEn) in PmControl

  //
  // Write them back
  //
  IoWrite16 (AcpiBaseAddr, Pm1Status);
  IoWrite16 ((UINT16)(AcpiBaseAddr + R_FCH_ACPI_PM_CONTROL), PmControl);
}

/**
  This is the entrypoint of PEIM

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
PeiInitPlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;

  Status = (**PeiServices).InstallPpi (PeiServices, &mPpiStall[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Dtermine boot mode and return boot mode.
  //
  BootMode = 0;
  Status   = UpdateBootMode (PeiServices, &BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // Install Boot mode ppi.
  //
  if (!EFI_ERROR (Status)) {
    Status = (**PeiServices).InstallPpi (PeiServices, &mPpiBootMode[0]);
    ASSERT_EFI_ERROR (Status);
  }

  if ((BootMode != BOOT_ON_S3_RESUME)) {
    //
    // Clear all pending SMI. On S3 clear power button enable so it wll not generate an SMI
    //
    ClearSmiAndWake ();
    ClearAllSmiControlRegisters ();
  }

  //
  // Notify for memory discovery callback
  //
  Status = (**PeiServices).NotifyPpi (PeiServices, &mMemoryDiscoveredNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
