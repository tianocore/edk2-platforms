/** @file

  Generate ACPI HPET table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>

STATIC
EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER  mAcpiHpet = {
  {
    EFI_ACPI_6_5_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER),
    EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION,
    0x00,
    { 0,                          0,    0, 0,                      0, 0 },
    0,
    0,
    0,
    0
  },
  /// EventTimerBlockId
  0,
  /// BaseAddressLower32Bit
  { EFI_ACPI_6_5_SYSTEM_MEMORY, 0x40, 0, EFI_ACPI_RESERVED_BYTE, 0 },
  /// HpetNumber
  0,
  /// MainCounterMinimumClockTickInPeriodicMode
  0,
  /// PageProtectionAndOemAttribute
  EFI_ACPI_NO_PAGE_PROTECTION
};

/**
  Implementation of AcpiHpetLibConstructor for AMD platforms.
  This is library constructor for AcpiHpetLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Table installation successful.
  @retval EFI_NOT_FOUND   HPET Capabilities register read failed.
  @retval Others          Table installation failed.
**/
EFI_STATUS
EFIAPI
AcpiHpetLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol;
  EFI_STATUS               Status;
  UINT64                   HpetCabalitiesId;
  UINTN                    NewTableKey;

  /// HPET Table Generation code goes here
  DEBUG ((DEBUG_INFO, "Generating ACPI HPET Table.\n"));

  /// Get HPET Capabilities ID register value and test if HPET is enabled
  HpetCabalitiesId = MmioRead64 (PcdGet32 (PcdHpetBaseAddress));

  /// If mmio address is not mapped
  if (HpetCabalitiesId == MAX_UINT64) {
    DEBUG ((DEBUG_ERROR, "HPET Capabilities register read failed.\n"));
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return EFI_NOT_FOUND;
  }

  ///
  /// Fill the Event Timer Block ID
  /// First 32-bit contains Event Timer Block ID
  ///
  mAcpiHpet.EventTimerBlockId = (UINT32)HpetCabalitiesId;

  /// Fill the Base Address
  mAcpiHpet.BaseAddressLower32Bit.Address = PcdGet32 (PcdHpetBaseAddress);

  ///
  /// Sequence number of the HPET
  /// This should match with ACPI HPET SSDT table _UID value of HPET device
  ///
  mAcpiHpet.HpetNumber = 0;

  /// Minimum clock tick in periodic mode
  mAcpiHpet.MainCounterMinimumClockTickInPeriodicMode = (UINT16)(HpetCabalitiesId >> 32);

  /// Update HPET Table Header
  mAcpiHpet.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mAcpiHpet.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  mAcpiHpet.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mAcpiHpet.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  CopyMem (&mAcpiHpet.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mAcpiHpet.Header.OemId));

  /// Locate ACPI Table Protocol
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI Table Protocol. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Install HPET Table
  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           &mAcpiHpet,
                           sizeof (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER),
                           &NewTableKey
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install HPET Table. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Implementation of AcpiHpetLibDestructor for AMD platforms.
  This is library destructor for AcpiHpetLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
AcpiHpetLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
