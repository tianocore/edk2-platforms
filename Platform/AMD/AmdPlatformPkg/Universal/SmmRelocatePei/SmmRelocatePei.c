/** @file
  The PEI driver for SMRAM space location.

  Copyright (C) 2024 - 2026, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmmRelocatePei.h"
#include <Ppi/AmdCpmTablePpi/AmdCpmTablePpi.h>

//
// Notification object for registering the callback, for when
// EDKII_PEI_MP_SERVICES2_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR  mMpServices2Notify = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH |   // Flags
  EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEdkiiPeiMpServices2PpiGuid,              // Guid
  OnMpServices2Available                     // Notify
};

/**
  To detect system already enable Modern Standby feature or not.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.

  @retval   TRUE    System enabled Modern Standby feature.
  @retval   FALSE   The Modern Standby feature not enabled.

**/
BOOLEAN
IsModernStandbyEnabled (
  IN EFI_PEI_SERVICES  **PeiServices
  )
{
  AMD_CPM_TABLE_PPI  *CpmTablePtr;

  //
  // Check for S3 support.  EFI_NOT_FOUND is a valid result meaning Modern
  // Standby is not enabled; no assertion on the return status.
  //
  CpmTablePtr = NULL;
  // EDKII implementation constrain to fix cert_exp40_c.
  /* coverity[cert_exp40_c_violation] */
  (VOID)(*PeiServices)->LocatePpi (
                          (const EFI_PEI_SERVICES **)PeiServices,
                          &gAmdCpmTablePpiGuid,
                          0,
                          NULL,
                          (VOID **)&CpmTablePtr
                          );
  if (CpmTablePtr != NULL) {
    return TRUE;
  }

  return FALSE;
}

/**
  Notification function called when EDKII_PEI_MP_SERVICES2_PPI becomes available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @retval  EFI_SUCCESS        The status code returned from this function is ignored.

**/
STATIC
EFI_STATUS
EFIAPI
OnMpServices2Available (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                      Status;
  EDKII_PEI_MP_SERVICES2_PPI      *MpServices2;
  VOID                            *GuidHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;
  VOID                            *AcpiReservedBase;
  RESERVED_ACPI_S3_RANGE          *AcpiS3Range;
  EFI_BOOT_MODE                   BootMode;
  UINTN                           S3MemSize;
  UINTN                           NumberOfProcessors;
  UINTN                           NumberOfEnabledProcessors;

  DEBUG ((DEBUG_INFO, "%a: %a\n", gEfiCallerBaseName, __func__));

  MpServices2 = Ppi;

  //
  // Smm Relocation Initialize.
  //
  Status = SmmRelocationInit (MpServices2);
  ASSERT_EFI_ERROR (Status);

  BootMode = 0;
  // EDKII implementation constrain to fix cert_exp40_c.
  /* coverity[cert_exp40_c_violation] */
  (*PeiServices)->GetBootMode ((const EFI_PEI_SERVICES **)PeiServices, &BootMode);

  if (BootMode != BOOT_ON_S3_RESUME) {
    if (IsModernStandbyEnabled (PeiServices)) {
      return EFI_SUCCESS;
    }

    //
    // Calculate memory size for S3
    //
    NumberOfProcessors = 0;
    MpServices2->GetNumberOfProcessors (MpServices2, &NumberOfProcessors, &NumberOfEnabledProcessors);
    S3MemSize = PcdGet32 (PcdPeiCoreMaxPeiStackSize) * 2 + (NumberOfProcessors + 1) * PcdGet32 (PcdCpuApStackSize) + PcdGet32 (PcdS3ExtraMemSize);
    DEBUG ((DEBUG_INFO, "NumberOfProcessors: 0x%X, S3MemSize = 0x%X\n", NumberOfProcessors, S3MemSize));
    //
    // Get Hob list for SMRAM desc
    //
    GuidHob         = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
    DescriptorBlock = GET_GUID_HOB_DATA (GuidHob);
    ASSERT (DescriptorBlock != NULL);

    //
    // Now find the location of the data structure that is used to store the address
    // of the S3 reserved memory.
    //
    AcpiS3Range = (RESERVED_ACPI_S3_RANGE *)(UINTN)(DescriptorBlock->Descriptor[0].PhysicalStart + RESERVED_ACPI_S3_RANGE_OFFSET);
    DEBUG ((DEBUG_INFO, "AcpiS3Range: 0x%X\n", (UINTN)AcpiS3Range));

    //
    // Allocate reserved ACPI memory for S3 resume.  Pointer to this region is
    // stored in SMRAM in the first page of TSEG.
    //
    AcpiReservedBase = AllocateReservedPages (EFI_SIZE_TO_PAGES (S3MemSize));
    DEBUG ((DEBUG_INFO, "AcpiReservedBase: 0x%X\n", (UINTN)AcpiReservedBase));
    ASSERT (AcpiReservedBase != NULL);
    if (AcpiReservedBase != NULL) {
      AcpiS3Range->AcpiReservedMemoryBase = (UINT32)(UINTN)AcpiReservedBase;
      AcpiS3Range->AcpiReservedMemorySize = (UINT32)S3MemSize;

      DEBUG ((DEBUG_INFO, "Reserved Memory for S3 Base:   0x%X\n", AcpiS3Range->AcpiReservedMemoryBase));
      DEBUG ((DEBUG_INFO, "Reserved Memory for S3 Size:   0x%X\n", AcpiS3Range->AcpiReservedMemorySize));
    }
  }

  return EFI_SUCCESS;
}

/**
  This is the entry point of PEIM.

  @param[in]  FileHandle  Handle of the file being invoked.
  @param[in]  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.

**/
EFI_STATUS
EFIAPI
RelocateSmBasePeiEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = (*PeiServices)->RegisterForShadow (FileHandle);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = (*PeiServices)->NotifyPpi ((const EFI_PEI_SERVICES **)PeiServices, &mMpServices2Notify);
  DEBUG ((DEBUG_INFO, "%a - Exit Status=%r\n", __func__, Status));
  return Status;
}
