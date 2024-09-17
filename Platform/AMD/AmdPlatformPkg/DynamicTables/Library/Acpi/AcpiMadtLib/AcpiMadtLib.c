/** @file

  Generate ACPI MADT table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>
#include <Protocol/MpService.h>
#include <Library/AmdPlatformSocLib.h>
#include <Library/LocalApicLib.h>
#include <Library/SortLib.h>

UINT32  mMadtCcdOrder[16] = { 0, 4, 8, 12, 2, 6, 10, 14, 3, 7, 11, 15, 1, 5, 9, 13 };

/**
  Callback compare function.
  Compares CCD number of provided arguments.

  @param[in] ProcessorInfoLeft   Pointer to Left Buffer.
  @param[in] ProcessorInfoRight  Pointer to Right Buffer.
  @return    0                 If both are same
             -1                If left value is less than righ value.
             1                 If left value is greater than righ value.

**/
INTN
EFIAPI
MadtSortByCcd (
  CONST VOID  *ProcessorInfoLeft,
  CONST VOID  *ProcessorInfoRight
  )
{
  CONST EFI_PROCESSOR_INFORMATION  *Left;
  CONST EFI_PROCESSOR_INFORMATION  *Right;
  UINT32                           Index;
  UINT32                           LeftCcdIndex;
  UINT32                           RightCcdIndex;

  Left  = (EFI_PROCESSOR_INFORMATION *)ProcessorInfoLeft;
  Right = (EFI_PROCESSOR_INFORMATION *)ProcessorInfoRight;

  // Get the CCD Index number
  LeftCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mMadtCcdOrder); Index++) {
    if (Left->ExtendedInformation.Location2.Die == mMadtCcdOrder[Index]) {
      LeftCcdIndex = Index;
      break;
    }
  }

  RightCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mMadtCcdOrder); Index++) {
    if (Right->ExtendedInformation.Location2.Die == mMadtCcdOrder[Index]) {
      RightCcdIndex = Index;
      break;
    }
  }

  // Now compare for quick sort
  if (LeftCcdIndex < RightCcdIndex) {
    return -1;
  }

  if (LeftCcdIndex > RightCcdIndex) {
    return 1;
  }

  return 0;
}

/**
  Get local NMI information for AMD platforms.

  @param[in, out] LocalNmi     Pointer to the Local NMI information.
  @param[in, out] LocalNmiSize Size of the Local NMI information.

  @retval EFI_SUCCESS           Successfully got Local NMI information.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Local NMI information.
  @retval EFI_INVALID_PARAMETER LocalNmi or LocalNmiSize is NULL.
**/
EFI_STATUS
GenerateLocalNmi (
  IN OUT UINT8  **LocalNmi,
  IN OUT UINTN  *LocalNmiSize
  )
{
  EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE    *LocalApicNmiInfo;
  EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE  *LocalX2ApicNmiInfo;

  if ((LocalNmi == NULL) || (LocalNmiSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (LOCAL_APIC_MODE_X2APIC == GetApicMode ()) {
    LocalX2ApicNmiInfo = AllocateZeroPool (sizeof (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE));
    if (LocalX2ApicNmiInfo == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed to allocate memory for Local NMI Structure.\n"));
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    LocalX2ApicNmiInfo->Type             = EFI_ACPI_6_5_LOCAL_X2APIC_NMI;
    LocalX2ApicNmiInfo->Length           = sizeof (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE);
    LocalX2ApicNmiInfo->Flags            = 0x5;
    LocalX2ApicNmiInfo->AcpiProcessorUid = 0xFFFFFFFF;
    LocalX2ApicNmiInfo->LocalX2ApicLint  = 0x1;
    LocalX2ApicNmiInfo->Reserved[0]      = 0;
    LocalX2ApicNmiInfo->Reserved[1]      = 0;
    LocalX2ApicNmiInfo->Reserved[2]      = 0;
    *LocalNmi                            = (UINT8 *)LocalX2ApicNmiInfo;
    *LocalNmiSize                        = sizeof (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE);
  } else {
    LocalApicNmiInfo = AllocateZeroPool (sizeof (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE));
    if (LocalApicNmiInfo == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed to allocate memory for Local NMI Structure.\n"));
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    LocalApicNmiInfo->Type             = EFI_ACPI_6_5_LOCAL_APIC_NMI;
    LocalApicNmiInfo->Length           = sizeof (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE);
    LocalApicNmiInfo->AcpiProcessorUid = 0xFF;
    LocalApicNmiInfo->Flags            = 0x5;
    LocalApicNmiInfo->LocalApicLint    = 0x1;
    *LocalNmi                          = (UINT8 *)LocalApicNmiInfo;
    *LocalNmiSize                      = sizeof (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE);
  }

  return EFI_SUCCESS;
}

/**
  Get Interrupt Source Override information for AMD platforms.

  @param[in, out] InterruptSourceOverride     Pointer to the Interrupt Source Override information.
  @param[in, out] InterruptSourceOverrideSize Size of the Interrupt Source Override information.

  @retval EFI_SUCCESS           Successfully got Interrupt Source Override information.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Interrupt Source Override information.

**/
EFI_STATUS
GenerateinterruptSourceOverride (
  IN OUT EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE  **InterruptSourceOverride,
  IN OUT UINTN                                             *InterruptSourceOverrideSize
  )
{
  EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE  *InterruptSourceOverrideInfo;

  if ((InterruptSourceOverride == NULL) || (InterruptSourceOverrideSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  InterruptSourceOverrideInfo = AllocateZeroPool (
                                  2 * sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE)
                                  );
  if (InterruptSourceOverrideInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for Interrupt Source Override Structure.\n"));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  InterruptSourceOverrideInfo[0].Type                  = EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE;
  InterruptSourceOverrideInfo[0].Length                = sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE);
  InterruptSourceOverrideInfo[0].Bus                   = 0x0;
  InterruptSourceOverrideInfo[0].Source                = 0x0;
  InterruptSourceOverrideInfo[0].GlobalSystemInterrupt = 0x2;
  InterruptSourceOverrideInfo[0].Flags                 = 0xF;

  InterruptSourceOverrideInfo[1].Type                  = EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE;
  InterruptSourceOverrideInfo[1].Length                = sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE);
  InterruptSourceOverrideInfo[1].Bus                   = 0x0;
  InterruptSourceOverrideInfo[1].Source                = 0x9;
  InterruptSourceOverrideInfo[1].GlobalSystemInterrupt = 0x9;
  InterruptSourceOverrideInfo[1].Flags                 = 0xF;

  *InterruptSourceOverride     = InterruptSourceOverrideInfo;
  *InterruptSourceOverrideSize = 2 * sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE);
  return EFI_SUCCESS;
}

/**
  Get IO APIC information for AMD platforms.

  @param[in, out] IoApic        Pointer to the IO APIC information.
  @param[in, out] IoApicSize    Number of IO APICs.

  @retval EFI_SUCCESS           Successfully got IO APIC information.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for IO APIC information.
  @retval Others                Failed to get IO APIC information.
**/
EFI_STATUS
GenerateIoApicStructure (
  IN OUT EFI_ACPI_6_5_IO_APIC_STRUCTURE  **IoApic,
  IN OUT UINTN                           *IoApicSize
  )
{
  EFI_ACPI_6_5_IO_APIC_STRUCTURE  *IoApicInfo;
  EFI_STATUS                      Status;
  UINTN                           IoApicCount;

  if ((IoApic == NULL) || (IoApicSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  /// Get IOAPIC information
  IoApicInfo  = NULL;
  IoApicCount = 0;
  Status      = GetIoApicInfo (&IoApicInfo, (UINT8 *)&IoApicCount);
  if ((EFI_ERROR (Status) || (IoApicInfo == NULL) || (IoApicCount == 0))) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d GetIoApicInfo() failed. Status (%r).\n",
      __func__,
      __LINE__,
      Status
      ));
    return Status;
  }

  *IoApic     = IoApicInfo;
  *IoApicSize = IoApicCount * sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE);
  return EFI_SUCCESS;
}

/**
  Generate Processor Local APIC Structure for AMD platforms.

  @param[in, out] ProcessorLocalApic     Pointer to the generated Processor Local APIC Structure.
  @param[in, out] ProcessorLocalApicSize Size of the generated Processor Local APIC Structure.

  @retval EFI_SUCCESS           Successfully generated Processor Local APIC Structure.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Processor Local APIC Structure.
  @retval Others                Failed to generate Processor Local APIC Structure.
**/
EFI_STATUS
GenerateProcessorLocalApicStructure (
  IN OUT UINT8  **ProcessorLocalApic,
  IN OUT UINTN  *ProcessorLocalApicSize
  )
{
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE    *DstApic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE    *LocalApic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE    *SortedItemApic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE    *SrcApic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE  *DstX2Apic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE  *LocalX2Apic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE  *SortedItemX2Apic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE  *SrcX2Apic;
  EFI_MP_SERVICES_PROTOCOL                       *MpService;
  EFI_PROCESSOR_INFORMATION                      *ProcessorInfoBuffer;
  EFI_STATUS                                     Status;
  UINTN                                          Index;
  UINTN                                          LocalApicSize;
  UINTN                                          NumberOfEnabledProcessors;
  UINTN                                          NumberOfProcessors;
  UINTN                                          NumSocket;
  UINTN                                          Socket;
  UINTN                                          ThreadsPerCore;

  /// Locate MP Services protocol
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpService);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to locate MP Services Protocol. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Get number of processors
  Status = MpService->GetNumberOfProcessors (
                        MpService,
                        &NumberOfProcessors,
                        &NumberOfEnabledProcessors
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to get number of processors. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ProcessorInfoBuffer = AllocateZeroPool (NumberOfProcessors * sizeof (EFI_PROCESSOR_INFORMATION));
  if (ProcessorInfoBuffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to allocate memory for Processor Information Buffer. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  /// Get ProcessorInfoBuffer
  NumSocket      = 0;
  ThreadsPerCore = 0;
  for (Index = 0; Index < NumberOfProcessors; Index++) {
    Status = MpService->GetProcessorInfo (
                          MpService,
                          Index|CPU_V2_EXTENDED_TOPOLOGY,
                          &ProcessorInfoBuffer[Index]
                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%d Failed to get Processor Information. Status(%r)\n",
        __func__,
        __LINE__,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      FreePool (ProcessorInfoBuffer);
      return Status;
    }

    if (ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Package > NumSocket) {
      NumSocket = ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Package;
    }

    if (ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Thread > ThreadsPerCore) {
      ThreadsPerCore = ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Thread;
    }
  }

  /// increment the NumSocket and ThreadsPerCore by 1, as it is 0 based
  NumSocket++;
  ThreadsPerCore++;

  /// Sort by CCD location
  if (NumSocket > 1) {
    PerformQuickSort (
      ProcessorInfoBuffer,
      NumberOfProcessors/2,
      sizeof (EFI_PROCESSOR_INFORMATION),
      MadtSortByCcd
      );
    PerformQuickSort (
      ProcessorInfoBuffer+(NumberOfProcessors/2),
      NumberOfProcessors/2,
      sizeof (EFI_PROCESSOR_INFORMATION),
      MadtSortByCcd
      );
  } else {
    PerformQuickSort (
      ProcessorInfoBuffer,
      NumberOfProcessors,
      sizeof (EFI_PROCESSOR_INFORMATION),
      MadtSortByCcd
      );
  }

  LocalApic   = NULL;
  LocalX2Apic = NULL;
  /// check whether in APIC mode or x2APIC mode
  if (LOCAL_APIC_MODE_X2APIC == GetApicMode ()) {
    /// Allocate memory for processor local x2APIC structure
    LocalApicSize = NumberOfProcessors * sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE);
    LocalX2Apic   = AllocateZeroPool (LocalApicSize);
    if (LocalX2Apic == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%d Failed to allocate memory for Processor Local X2APIC Structure. Status(%r)\n",
        __func__,
        __LINE__,
        Status
        ));
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      FreePool (ProcessorInfoBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    /// Allocate memory for Processor Local APIC Structure
    LocalApicSize = NumberOfProcessors * sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE);
    LocalApic     = AllocateZeroPool (LocalApicSize);
    if (LocalApic == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%d Failed to allocate memory for Processor Local APIC Structure. Status(%r)\n",
        __func__,
        __LINE__,
        Status
        ));
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      FreePool (ProcessorInfoBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  for (Socket = 0; Socket < NumSocket; Socket++) {
    for (Index = 0; Index < NumberOfProcessors; Index++) {
      if ((ProcessorInfoBuffer[Index].StatusFlag & PROCESSOR_ENABLED_BIT) != 0) {
        if (LOCAL_APIC_MODE_X2APIC == GetApicMode ()) {
          LocalX2Apic[Index].Type             = EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC;
          LocalX2Apic[Index].Length           = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE);
          LocalX2Apic[Index].Reserved[0]      = 0;
          LocalX2Apic[Index].Reserved[1]      = 0;
          LocalX2Apic[Index].X2ApicId         = (UINT32)ProcessorInfoBuffer[Index].ProcessorId;
          LocalX2Apic[Index].Flags            = 1;
          LocalX2Apic[Index].AcpiProcessorUid = (UINT32)Index;
        } else {
          LocalApic[Index].Type             = EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC;
          LocalApic[Index].Length           = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE);
          LocalApic[Index].ApicId           = (UINT8)ProcessorInfoBuffer[Index].ProcessorId;
          LocalApic[Index].Flags            = 1;
          LocalApic[Index].AcpiProcessorUid = (UINT8)Index;
        }
      } else {
        if (LOCAL_APIC_MODE_X2APIC == GetApicMode ()) {
          LocalX2Apic[Index].Type             = EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC;
          LocalX2Apic[Index].Length           = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE);
          LocalX2Apic[Index].Reserved[0]      = 0;
          LocalX2Apic[Index].Reserved[1]      = 0;
          LocalX2Apic[Index].X2ApicId         = MAX_UINT32;
          LocalX2Apic[Index].Flags            = 0;
          LocalX2Apic[Index].AcpiProcessorUid = MAX_UINT32;
        } else {
          LocalApic[Index].Type             = EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC;
          LocalApic[Index].Length           = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE);
          LocalApic[Index].AcpiProcessorUid = MAX_UINT8;
          LocalApic[Index].ApicId           = MAX_INT8;
          LocalApic[Index].Flags            = 0;
        }
      }
    }
  }

  FreePool (ProcessorInfoBuffer);

  /// Now separate the second thread list
  if (ThreadsPerCore > 1) {
    if (LOCAL_APIC_MODE_X2APIC == GetApicMode ()) {
      SortedItemX2Apic = NULL;
      SortedItemX2Apic = AllocateZeroPool (
                           sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE) * NumberOfProcessors
                           );
      if (SortedItemX2Apic == NULL) {
        FreePool (LocalX2Apic);
        ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
        return EFI_OUT_OF_RESOURCES;
      }

      SrcX2Apic = LocalX2Apic;
      DstX2Apic = SortedItemX2Apic;
      for (Index = 0; Index < NumberOfProcessors; Index++) {
        if ((SrcX2Apic->X2ApicId & 0x1) == 0) {
          CopyMem (DstX2Apic, SrcX2Apic, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE));
          SrcX2Apic++;
          DstX2Apic++;
        } else {
          SrcX2Apic++;
        }
      }

      SrcX2Apic = LocalX2Apic;
      for (Index = 0; Index < NumberOfProcessors; Index++) {
        if ((SrcX2Apic->X2ApicId & 0x1) == 1) {
          CopyMem (DstX2Apic, SrcX2Apic, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE));
          SrcX2Apic++;
          DstX2Apic++;
        } else {
          SrcX2Apic++;
        }
      }

      CopyMem (
        LocalX2Apic,
        SortedItemX2Apic,
        sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE) * NumberOfProcessors
        );
      FreePool (SortedItemX2Apic);
    } else {
      SortedItemApic = NULL;
      SortedItemApic = AllocateZeroPool (
                         sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE) * NumberOfProcessors
                         );
      if (SortedItemApic == NULL) {
        FreePool (LocalApic);
        ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
        return EFI_OUT_OF_RESOURCES;
      }

      SrcApic = LocalApic;
      DstApic = SortedItemApic;
      for (Index = 0; Index < NumberOfProcessors; Index++) {
        if ((SrcApic->ApicId & 0x1) == 0) {
          CopyMem (DstApic, SrcApic, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE));
          SrcApic++;
          DstApic++;
        } else {
          SrcApic++;
        }
      }

      SrcApic = LocalApic;
      for (Index = 0; Index < NumberOfProcessors; Index++) {
        if ((SrcApic->ApicId & 0x1) == 1) {
          CopyMem (DstApic, SrcApic, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE));
          SrcApic++;
          DstApic++;
        } else {
          SrcApic++;
        }
      }

      CopyMem (
        LocalApic,
        SortedItemApic,
        sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE) * NumberOfProcessors
        );
      FreePool (SortedItemApic);
    }
  }

  /// copy the data to arguments
  if (LOCAL_APIC_MODE_X2APIC == GetApicMode ()) {
    *ProcessorLocalApic = (UINT8 *)LocalX2Apic;
  } else {
    *ProcessorLocalApic = (UINT8 *)LocalApic;
  }

  *ProcessorLocalApicSize = LocalApicSize;

  return EFI_SUCCESS;
}

/**
  Generate ACPI MADT table for AMD platforms.

  @param[in, out] AcpiMadt  Pointer to the generated MADT Table Header.

  @retval EFI_SUCCESS           Successfully generated MADT Table.
  @retval EFI_INVALID_PARAMETER AcpiMadt is NULL.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for MADT Table.
**/
EFI_STATUS
GenerateMadtTableHeader (
  IN OUT EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  **AcpiMadt
  )
{
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *MadtTableHeader;

  if (AcpiMadt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  /// Allocate memory for MADT Table
  MadtTableHeader = AllocateZeroPool (sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER));
  if (MadtTableHeader == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to allocate memory for MADT Table.\n",
      __func__,
      __LINE__
      ));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  /// Initialize MADT Table Header
  MadtTableHeader->Header.Signature       = EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE;
  MadtTableHeader->Header.Length          = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  MadtTableHeader->Header.Revision        = EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION;
  MadtTableHeader->Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  MadtTableHeader->Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  MadtTableHeader->Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  MadtTableHeader->Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  CopyMem (
    &MadtTableHeader->Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (MadtTableHeader->Header.OemId)
    );

  /// Initialize Local APIC Address and Flags
  MadtTableHeader->LocalApicAddress = PcdGet32 (PcdCpuLocalApicBaseAddress);
  MadtTableHeader->Flags            = EFI_ACPI_6_5_PCAT_COMPAT;

  *AcpiMadt = MadtTableHeader;
  return EFI_SUCCESS;
}

/**
  Implementation of AcpiMadtLibConstructor for AMD platforms.
  This is library constructor for AcpiMadtLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated and installed MADT Table.
  @retval Others          Failed to generate and install MADT Table.
**/
EFI_STATUS
EFIAPI
AcpiMadtLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE     *InterruptSourceOverride;
  EFI_ACPI_6_5_IO_APIC_STRUCTURE                       *IoApic;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *AcpiMadt;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *AcpiMadtHeader;
  EFI_ACPI_TABLE_PROTOCOL                              *AcpiProtocol;
  EFI_STATUS                                           Status;
  UINT8                                                *AcpiMadtPtr;
  UINT8                                                *LocalNmi;
  UINT8                                                *ProcessorLocalApic;
  UINTN                                                HeaderSize;
  UINTN                                                InterruptSourceOverrideSize;
  UINTN                                                IoApicSize;
  UINTN                                                LocalNmiSize;
  UINTN                                                NewTableKey;
  UINTN                                                ProcessorLocalApicSize;
  UINTN                                                TableSize;

  /// MADT Table Generation code goes here
  DEBUG ((DEBUG_INFO, "Generating ACPI MADT Table.\n"));

  /// Locate ACPI Table Protocol
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to locate ACPI Table Protocol. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  TableSize = 0;
  AcpiMadt  = NULL;
  Status    = GenerateMadtTableHeader (&AcpiMadtHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to generate MADT Table Header. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  HeaderSize = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  TableSize += HeaderSize;

  /// Generate Processor Local APIC Structure
  Status = GenerateProcessorLocalApicStructure (&ProcessorLocalApic, &ProcessorLocalApicSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to generate Processor Local APIC Structure. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    FreePool (AcpiMadtHeader);
    return Status;
  }

  TableSize += ProcessorLocalApicSize;

  /// Generate IO APIC Structure
  Status = GenerateIoApicStructure (&IoApic, &IoApicSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to generate IO APIC Structure. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    FreePool (ProcessorLocalApic);
    FreePool (AcpiMadtHeader);
    return Status;
  }

  TableSize += IoApicSize;

  /// Generate Interrupt Source Override Structure
  Status = GenerateinterruptSourceOverride (&InterruptSourceOverride, &InterruptSourceOverrideSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to generate Interrupt Source Override Structure. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    FreePool (IoApic);
    FreePool (ProcessorLocalApic);
    FreePool (AcpiMadtHeader);
    return Status;
  }

  TableSize += InterruptSourceOverrideSize;

  /// Generate Local NMI Structure
  Status = GenerateLocalNmi (&LocalNmi, &LocalNmiSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to generate Local NMI Structure. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    FreePool (InterruptSourceOverride);
    FreePool (IoApic);
    FreePool (ProcessorLocalApic);
    FreePool (AcpiMadtHeader);
    return Status;
  }

  TableSize += LocalNmiSize;

  AcpiMadt = AllocateZeroPool (TableSize);
  if (AcpiMadt == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to allocate memory for MADT Table.\n",
      __func__,
      __LINE__
      ));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    FreePool (InterruptSourceOverride);
    FreePool (IoApic);
    FreePool (ProcessorLocalApic);
    FreePool (AcpiMadtHeader);
    return EFI_OUT_OF_RESOURCES;
  }

  AcpiMadtPtr = (UINT8 *)AcpiMadt;
  CopyMem (AcpiMadtPtr, AcpiMadtHeader, HeaderSize);
  FreePool (AcpiMadtHeader);
  AcpiMadtPtr += HeaderSize;

  CopyMem (AcpiMadtPtr, ProcessorLocalApic, ProcessorLocalApicSize);
  FreePool (ProcessorLocalApic);
  AcpiMadtPtr += ProcessorLocalApicSize;

  CopyMem (AcpiMadtPtr, IoApic, IoApicSize);
  FreePool (IoApic);
  AcpiMadtPtr += IoApicSize;

  CopyMem (AcpiMadtPtr, InterruptSourceOverride, InterruptSourceOverrideSize);
  FreePool (InterruptSourceOverride);
  AcpiMadtPtr += InterruptSourceOverrideSize;

  CopyMem (AcpiMadtPtr, LocalNmi, LocalNmiSize);
  FreePool (LocalNmi);
  AcpiMadtPtr += LocalNmiSize;

  AcpiMadt->Header.Length = TableSize;
  /// Install MADT Table
  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           AcpiMadt,
                           TableSize,
                           &NewTableKey
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install MADT Table. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  FreePool (AcpiMadt);
  return Status;
}

/**
  Implementation of AcpiMadtLibDestructor for AMD platforms.
  This is library destructor for AcpiMadtLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
AcpiMadtLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
