/** @file
  BoardInitLib library internal implementation for DXE phase.

  Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeBoardInitLibInternal.h"
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <PiDxe.h>

#define FADT_SMI_CMD_PORT_LENGTH  1
#define FADT_PM1_EVT_BLK_LENGTH   4
#define FADT_PM1_CNT_BLK_LENGTH   2
#define FADT_PM_TMR_BLK_LENGTH    4
#define FADT_GPE0_BLK_LENGTH      8
#define FADT_RESET_REG_PORT       0xCF9
#define FADT_RESET_REG_LENGTH     1

/**
  Report an I/O range to GCD and allocate it if needed.

  @param[in]  ImageHandle  Handle of the DXE image reserving the range.
  @param[in]  BaseAddress  Base address of the I/O range.
  @param[in]  Length       Length of the I/O range.
  @param[in]  RangeName    Debug name of the I/O range.

  @retval EFI_SUCCESS            The I/O range is reserved in GCD.
  @retval EFI_INVALID_PARAMETER  RangeName is NULL.
  @retval Others                 Error returned by GCD services.
**/
EFI_STATUS
EFIAPI
ReportIoRangeToGcd (
  IN EFI_HANDLE   ImageHandle,
  IN UINT64       BaseAddress,
  IN UINT64       Length,
  IN CONST CHAR8  *RangeName
  )
{
  EFI_GCD_IO_SPACE_DESCRIPTOR  IoSpaceDescriptor;
  EFI_PHYSICAL_ADDRESS         IoBaseAddress;
  EFI_STATUS                   CalledStatus;
  EFI_STATUS                   Status;

  if ((RangeName == NULL) || (BaseAddress == 0) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  CalledStatus = gDS->GetIoSpaceDescriptor (BaseAddress, &IoSpaceDescriptor);
  if (EFI_ERROR (CalledStatus) || (IoSpaceDescriptor.GcdIoType != EfiGcdIoTypeIo)) {
    CalledStatus = gDS->AddIoSpace (EfiGcdIoTypeIo, BaseAddress, Length);
    if (EFI_ERROR (CalledStatus)) {
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: Failed to add %a I/O range. Base = 0x%Lx Length = 0x%Lx Status = %r\n",
         RangeName,
         BaseAddress,
         Length,
         CalledStatus
        )
        );
      Status = CalledStatus;
    } else {
      DEBUG (
        (
         DEBUG_INFO,
         "INFO: Added %a I/O range. Base = 0x%Lx Length = 0x%Lx\n",
         RangeName,
         BaseAddress,
         Length
        )
        );
      // AddIoSpace always creates an unallocated entry (ImageHandle == NULL).
      // A second GetIoSpaceDescriptor call is not needed; set ImageHandle directly
      // so the AllocateIoSpace guard below evaluates correctly.
      IoSpaceDescriptor.ImageHandle = NULL;
    }
  }

  if (!EFI_ERROR (CalledStatus) && (IoSpaceDescriptor.ImageHandle == NULL)) {
    IoBaseAddress = BaseAddress;
    CalledStatus  = gDS->AllocateIoSpace (
                           EfiGcdAllocateAddress,
                           EfiGcdIoTypeIo,
                           0,
                           Length,
                           &IoBaseAddress,
                           ImageHandle,
                           NULL
                           );
    if (EFI_ERROR (CalledStatus)) {
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: Failed to allocate %a I/O range. Base = 0x%Lx Length = 0x%Lx Status = %r\n",
         RangeName,
         IoBaseAddress,
         Length,
         CalledStatus
        )
        );
      Status = CalledStatus;
    } else {
      DEBUG (
        (
         DEBUG_INFO,
         "INFO: Allocated %a I/O range. Base = 0x%Lx Length = 0x%Lx\n",
         RangeName,
         IoBaseAddress,
         Length
        )
        );
    }
  }

  return Status;
}

/**
  Reserve Legacy VGA IO space.

  @retval  EFI_SUCCESS  MMIO at Legacy VGA region has been allocated.
  @retval  !EFI_SUCCESS Error allocating the legacy VGA region.

**/
EFI_STATUS
EFIAPI
ReserveLegacyVgaIoSpace (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  VgaMemAddress;

  VgaMemAddress = (EFI_PHYSICAL_ADDRESS)VGA_MEM_BASE;
  Status        = gBS->AllocatePages (
                         AllocateAddress,
                         EfiMemoryMappedIO,
                         EFI_SIZE_TO_PAGES (VGA_MEM_SIZE),
                         &VgaMemAddress
                         );
  return Status;
}

/**
  Report the FADT I/O resources to GCD and allocate them if required.

  @param[in]  ImageHandle  Handle of the DXE image reserving the ranges.

  @retval EFI_SUCCESS      The FADT I/O resources are reserved in GCD.
  @retval Others           Error reserving one or more ranges.
**/
EFI_STATUS
EFIAPI
ReserveFadtIoResources (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  CalledStatus;

  CalledStatus = EFI_SUCCESS;

  Status = ReportIoRangeToGcd (
             ImageHandle,
             PcdGet16 (PcdAmdFchCfgSmiCmdPortAddr),
             FADT_SMI_CMD_PORT_LENGTH,
             "FADT.SmiCmd"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not report FADT.SmiCmd to GCD\n"));
    CalledStatus = Status;
  }

  Status = ReportIoRangeToGcd (
             ImageHandle,
             PcdGet16 (PcdAmdFchCfgAcpiPm1EvtBlkAddr),
             FADT_PM1_EVT_BLK_LENGTH,
             "FADT.Pm1aEvtBlk"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not report FADT.Pm1aEvtBlk to GCD\n"));
    CalledStatus = Status;
  }

  Status = ReportIoRangeToGcd (
             ImageHandle,
             PcdGet16 (PcdAmdFchCfgAcpiPm1CntBlkAddr),
             FADT_PM1_CNT_BLK_LENGTH,
             "FADT.Pm1aCntBlk"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not report FADT.Pm1aCntBlk to GCD\n"));
    CalledStatus = Status;
  }

  Status = ReportIoRangeToGcd (
             ImageHandle,
             PcdGet16 (PcdAmdFchCfgAcpiPmTmrBlkAddr),
             FADT_PM_TMR_BLK_LENGTH,
             "FADT.PmTmrBlk"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not report FADT.PmTmrBlk to GCD\n"));
    CalledStatus = Status;
  }

  Status = ReportIoRangeToGcd (
             ImageHandle,
             PcdGet16 (PcdAmdFchCfgAcpiGpe0BlkAddr),
             FADT_GPE0_BLK_LENGTH,
             "FADT.Gpe0Blk"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not report FADT.Gpe0Blk to GCD\n"));
    CalledStatus = Status;
  }

  Status = ReportIoRangeToGcd (
             ImageHandle,
             FADT_RESET_REG_PORT,
             FADT_RESET_REG_LENGTH,
             "FADT.ResetReg"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not report FADT.ResetReg to GCD\n"));
    CalledStatus = Status;
  }

  return CalledStatus;
}
