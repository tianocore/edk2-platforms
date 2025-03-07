/** @file
  Sample ACPI Platform Driver

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Statements that include other files.
//
#include "AcpiPlatformHooks.h"
#include <Protocol/GlobalNvsArea.h>

extern EFI_GLOBAL_NVS_AREA_PROTOCOL  mGlobalNvsArea;

/**
    Update the DSDT table.

    @param   TableHeader   The table to be set.

    @retval  EFI_SUCCESS   Update DSDT table sucessfully.

**/
EFI_STATUS
PatchDsdtTable (
  IN OUT   EFI_ACPI_DESCRIPTION_HEADER  *TableHeader
  )
{
  UINT8   *CurrPtr;
  UINT8   *DsdtPointer;
  UINT32  *Signature;
  UINT8   *Operation;
  UINT32  *Address;
  UINT16  *Size;

  //
  // Loop through the ASL looking for values that we must fix up.
  //
  CurrPtr = (UINT8 *)TableHeader;
  for (DsdtPointer = CurrPtr; DsdtPointer <= (CurrPtr + ((EFI_ACPI_COMMON_HEADER *)CurrPtr)->Length); DsdtPointer++) {
    Signature = (UINT32 *)DsdtPointer;

    switch (*Signature) {
      //
      // GNVS operation region.
      //
      case (SIGNATURE_32 ('G', 'N', 'V', 'S')):
        //
        // Conditional match.  For Region Objects, the Operator will always be the
        // byte immediately before the specific name.  Therefore, subtract 1 to check
        // the Operator.
        //
        Operation = DsdtPointer - 1;
        if (*Operation == AML_OPREGION_OP) {
          Address  = (UINT32 *)(DsdtPointer + 6);
          *Address = (UINT32)(UINTN)mGlobalNvsArea.Area;
          Size     = (UINT16 *)(DsdtPointer + 11);
          *Size    = sizeof (EFI_GLOBAL_NVS_AREA);
        }

        break;
      default:
        break;
    }
  }

  return EFI_SUCCESS;
}

/**
    Update the MADT table.

    @param  TableHeader   The table to be set.

    @retval  EFI_SUCCESS  Update MADT table sucessfully.

**/
EFI_STATUS
PatchMadtTable (
  IN OUT   EFI_ACPI_DESCRIPTION_HEADER  *TableHeader
  )
{
  EFI_STATUS                                   Status;
  EFI_MP_SERVICES_PROTOCOL                     *MpService;
  UINTN                                        NumCPUs = 1;
  UINTN                                        NumEnabledCPUs;
  UINT8                                        CurrProcessor = 0;
  EFI_PROCESSOR_INFORMATION                    ProcessorInfo;
  UINT8                                        *CurrPtr;
  EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_STRUCTURE  *ApicPtr   = NULL;
  EFI_ACPI_5_0_IO_APIC_STRUCTURE               *IoApicPtr = NULL;

  // Find the MP Protocol.
  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&MpService
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Determine the number of processors
  MpService->GetNumberOfProcessors (
               MpService,
               &NumCPUs,
               &NumEnabledCPUs
               );

  CurrPtr  = (UINT8 *)TableHeader;
  CurrPtr += sizeof (EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  while (CurrPtr < ((UINT8 *)TableHeader + ((EFI_ACPI_COMMON_HEADER *)TableHeader)->Length)) {
    // Local APIC
    ApicPtr = (EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_STRUCTURE *)CurrPtr;
    if (ApicPtr->Type == EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC) {
      // Disable at first
      ApicPtr->Flags  = 0;
      ApicPtr->ApicId = 0;

      // retrieve processor information
      Status = MpService->GetProcessorInfo (
                            MpService,
                            CurrProcessor,
                            &ProcessorInfo
                            );
      if (!EFI_ERROR (Status)) {
        if (ProcessorInfo.StatusFlag & PROCESSOR_ENABLED_BIT) {
          ApicPtr->Flags = EFI_ACPI_5_0_LOCAL_APIC_ENABLED;
        }

        ApicPtr->ApicId = (UINT8)(ProcessorInfo.ProcessorId);
      }

      // Increment the procesor count
      CurrProcessor++;
    }

    // IO APIC (IOHUB and FCH)
    IoApicPtr = (EFI_ACPI_5_0_IO_APIC_STRUCTURE *)CurrPtr;
    if (IoApicPtr->Type == EFI_ACPI_5_0_IO_APIC) {
      // IoApicPtr->IoApicId = PcdGet8 (PcdCfgFchIoapicId);
      // IoApicPtr->IoApicId = PcdGet8 (PcdCfgGnbIoapicId);
    }

    // Go to the next structure in the APIC table
    CurrPtr += (ApicPtr->Length);
  }

  return EFI_SUCCESS;
}
