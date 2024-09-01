/** @file
  This file patches the ACPI MADT table for AMD specific values.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeBoardInitLibInternal.h"
#include <Library/AmdPlatformSocLib.h>
#include <Library/LocalApicLib.h>
#define   AMD_CPUID_EXTENDED_TOPOLOGY_V2                 0x26
#define   AMD_CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_CCX  0x03
#define   AMD_CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_CCD  0x04
#define   AMD_CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE  0x05

UINT32  mCcdOrder[16] = { 0, 4, 8, 12, 2, 6, 10, 14, 3, 7, 11, 15, 1, 5, 9, 13 };

/**
  Callback compare function.
  Compares CCD number of provided arguments.

  @param[in] LocalX2ApicLeft   Pointer to Left Buffer.
  @param[in] LocalX2ApicRight  Pointer to Right Buffer.
  @return    0                 If both are same
             -1                If left value is less than righ value.
             1                 If left value is greater than righ value.

**/
INTN
EFIAPI
SortByCcd (
  CONST VOID  *LocalX2ApicLeft,
  CONST VOID  *LocalX2ApicRight
  )
{
  CONST EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE  *Left;
  CONST EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE  *Right;
  EFI_CPU_PHYSICAL_LOCATION2                           LeftLocation;
  EFI_CPU_PHYSICAL_LOCATION2                           RightLocation;
  UINT32                                               LeftCcdIndex;
  UINT32                                               RightCcdIndex;
  UINT32                                               Index;

  Left  = (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE *)LocalX2ApicLeft;
  Right = (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE *)LocalX2ApicRight;

  GetProcessorLocation2ByApicId (
    Left->X2ApicId,
    &LeftLocation.Package,
    &LeftLocation.Die,
    &LeftLocation.Tile,
    &LeftLocation.Module,
    &LeftLocation.Core,
    &LeftLocation.Thread
    );

  GetProcessorLocation2ByApicId (
    Right->X2ApicId,
    &RightLocation.Package,
    &RightLocation.Die,
    &RightLocation.Tile,
    &RightLocation.Module,
    &RightLocation.Core,
    &RightLocation.Thread
    );

  // Get the CCD Index number
  LeftCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mCcdOrder); Index++) {
    if (LeftLocation.Die == mCcdOrder[Index]) {
      LeftCcdIndex = Index;
      break;
    }
  }

  RightCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mCcdOrder); Index++) {
    if (RightLocation.Die == mCcdOrder[Index]) {
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
  A Callback function to patch the ACPI MADT table.
  Updates MADT table with AMD specific values, which
  are different than MinPlatformPkg.

  @param[in, out] NewTable       Pointer to ACPI MADT table

  @return         EFI_SUCCESS    Always return EFI_SUCCESSe

**/
EFI_STATUS
EFIAPI
MadtAcpiTablePatch (
  IN OUT  EFI_ACPI_SDT_HEADER  *NewTable
  )
{
  UINT32                                               Index;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *NewMadtTable;
  UINT8                                                *TablePtr;
  UINT64                                               Length;
  EFI_ACPI_6_5_IO_APIC_STRUCTURE                       *NbioIoApic;
  UINT8                                                IoApicCount;
  UINTN                                                LapicCount;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE        *LocalX2ApicPtr;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE        *SortedItem;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE        *Src;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE        *Dst;

  // Patch the Table
  NewMadtTable                  = (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)NewTable;
  NewMadtTable->Header.Revision = 6;
  // Get the IoApic information
  NbioIoApic     = NULL;
  IoApicCount    = 0;
  LapicCount     = 0;
  LocalX2ApicPtr = NULL;
  GetIoApicInfo (&NbioIoApic, &IoApicCount);
  if ((NbioIoApic == NULL) || (IoApicCount == 0)) {
    DEBUG ((DEBUG_INFO, "%a:%d Cannot obtain NBIO IOAPIC information.\n", __func__, __LINE__));
    return EFI_SUCCESS;
  }

  // Create MADT header
  TablePtr = (UINT8 *)NewMadtTable;
  TablePtr = TablePtr + sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  Length   = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  // Get the IOAPIC structure
  Index = 0;    // now holds the IoApic Index
  do {
    if (((STRUCTURE_HEADER *)TablePtr)->Type == EFI_ACPI_6_5_IO_APIC) {
      // Patch the IoApic Strucure
      if (Index >= IoApicCount) {
        /// Mark the extra IOAPIC structure Type as reserved, so that OSPM can ignore it.
        /// As per ACPI specification 6.5 for MADT table
        /// Subtype 0x18-0x7F are reserved, OSPM skips structures of the reserved type.
        ((EFI_ACPI_6_5_IO_APIC_STRUCTURE *)TablePtr)->Type = 0x7F;
      } else {
        ((EFI_ACPI_6_5_IO_APIC_STRUCTURE *)TablePtr)->IoApicId                  = NbioIoApic[Index].IoApicId;
        ((EFI_ACPI_6_5_IO_APIC_STRUCTURE *)TablePtr)->IoApicAddress             = NbioIoApic[Index].IoApicAddress;
        ((EFI_ACPI_6_5_IO_APIC_STRUCTURE *)TablePtr)->GlobalSystemInterruptBase = NbioIoApic[Index].GlobalSystemInterruptBase;
      }

      Index++;
    }

    if (((STRUCTURE_HEADER *)TablePtr)->Type == EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE) {
      // Patch Flags
      ((EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE *)TablePtr)->Flags = 0xF;
    }

    if (((STRUCTURE_HEADER *)TablePtr)->Type == EFI_ACPI_6_5_LOCAL_X2APIC_NMI) {
      // Patch  Flags - Edge-triggered, Active High
      ((EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE *)TablePtr)->Flags = 0x0005;
    }

    if (((STRUCTURE_HEADER *)TablePtr)->Type == EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC) {
      if (LapicCount == 0) {
        // Get the first entry pointer
        LocalX2ApicPtr = (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE *)TablePtr;
      }

      LapicCount += 1;
    }

    Length   += ((STRUCTURE_HEADER *)TablePtr)->Length;
    TablePtr += ((STRUCTURE_HEADER *)TablePtr)->Length;
  } while (Length < NewMadtTable->Header.Length);

  FreePool (NbioIoApic);

  if (LocalX2ApicPtr != NULL) {
    if (FixedPcdGet32 (PcdMaxCpuSocketCount) > 1) {
      /// Sort by CCD location
      PerformQuickSort (LocalX2ApicPtr, LapicCount/2, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE), SortByCcd);
      PerformQuickSort (LocalX2ApicPtr+(LapicCount/2), LapicCount/2, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE), SortByCcd);
    } else {
      /// Sort by CCD location
      PerformQuickSort (LocalX2ApicPtr, LapicCount, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE), SortByCcd);
    }

    /// Now allocate the Uid
    SortedItem = LocalX2ApicPtr;
    for (Index = 0; Index < LapicCount; Index++, SortedItem++) {
      SortedItem->AcpiProcessorUid = Index;
    }

    // Now separate the second thread list
    SortedItem = LocalX2ApicPtr + 1;
    if ((SortedItem->X2ApicId & 0x1) == 0x1) {
      // It has multi-thread on
      SortedItem = NULL;
      SortedItem = AllocateZeroPool (sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE) * LapicCount);
      if (SortedItem == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Src = LocalX2ApicPtr;
      Dst = SortedItem;
      for (Index = 0; Index < LapicCount; Index++) {
        if ((Src->X2ApicId & 0x1) == 0) {
          CopyMem (Dst, Src, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE));
          Src++;
          Dst++;
        } else {
          Src++;
        }
      }

      Src = LocalX2ApicPtr;
      for (Index = 0; Index < LapicCount; Index++) {
        if ((Src->X2ApicId & 0x1) == 1) {
          CopyMem (Dst, Src, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE));
          Src++;
          Dst++;
        } else {
          Src++;
        }
      }

      CopyMem (LocalX2ApicPtr, SortedItem, sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE) * LapicCount);
      FreePool (SortedItem);
    }
  }

  return EFI_SUCCESS;
}
