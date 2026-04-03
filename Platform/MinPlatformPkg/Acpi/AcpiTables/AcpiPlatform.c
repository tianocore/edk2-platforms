/** @file
  ACPI Platform Driver

Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
Copyright (c) 2021, AMD Incorporated. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

#pragma pack(1)

typedef struct {
  UINT32   AcpiProcessorUid;
  UINT32   ApicId;
  UINT32   Flags;
  UINT32   SocketNum;
  UINT32   Thread;
  UINT8    CoreType;
} EFI_CPU_ID_ORDER_MAP;

//
// Private Driver Data
//
//
// Define Union of IO APIC & Local APIC structure;
//
typedef union {
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE   AcpiLocalApic;
  EFI_ACPI_6_5_IO_APIC_STRUCTURE                AcpiIoApic;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE AcpiLocalx2Apic;
  struct {
    UINT8 Type;
    UINT8 Length;
  } AcpiApicCommon;
} ACPI_APIC_STRUCTURE_PTR;

#pragma pack()

extern EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE     Facs;
extern EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE        Fadt;
extern EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER Hpet;
extern EFI_ACPI_WSMT_TABLE Wsmt;

VOID  *mLocalTable[] = {
  &Facs,
  &Fadt,
  &Hpet,
  &Wsmt,
};

EFI_ACPI_TABLE_PROTOCOL     *mAcpiTable;

UINT32                      mNumOfBitShift = 6;
BOOLEAN                     mX2ApicEnabled;

EFI_MP_SERVICES_PROTOCOL    *mMpService;
UINTN                       mNumberOfCpus = 0;
UINTN                       mNumberOfEnabledCPUs = 0;

/**
  Print Cpu Apic ID Table

  @param[in]  CpuApicIdOrderTable       Data will be dumped.
**/
VOID
DebugDisplayReOrderTable (
  IN EFI_CPU_ID_ORDER_MAP *CpuApicIdOrderTable
  )
{
  UINT32 Index;

  DEBUG ((DEBUG_INFO, "Index  AcpiProcId  ApicId   Thread  Flags   Skt  CoreType\n"));
  for (Index = 0; Index < mNumberOfCpus; Index++) {
    DEBUG ((DEBUG_INFO, " %02d       0x%02X      0x%02X       %d      %d      %d      0x%x\n",
                           Index,
                           CpuApicIdOrderTable[Index].AcpiProcessorUid,
                           CpuApicIdOrderTable[Index].ApicId,
                           CpuApicIdOrderTable[Index].Thread,
                           CpuApicIdOrderTable[Index].Flags,
                           CpuApicIdOrderTable[Index].SocketNum,
                           CpuApicIdOrderTable[Index].CoreType));
  }
}

EFI_STATUS
AppendCpuMapTableEntry (
    IN VOID   *ApicPtr,
    IN UINT32 LocalApicCounter,
    IN EFI_CPU_ID_ORDER_MAP *CpuApicIdOrderTable
  )
{
  EFI_STATUS    Status;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE   *LocalApicPtr;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE *LocalX2ApicPtr;
  UINT8         Type;

  Status = EFI_SUCCESS;
  Type = ((ACPI_APIC_STRUCTURE_PTR *)ApicPtr)->AcpiApicCommon.Type;
  LocalApicPtr = (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE *)(&((ACPI_APIC_STRUCTURE_PTR *)ApicPtr)->AcpiLocalApic);
  LocalX2ApicPtr = (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE *)(&((ACPI_APIC_STRUCTURE_PTR *)ApicPtr)->AcpiLocalx2Apic);

  if(Type == EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC) {
    if(!mX2ApicEnabled) {
      LocalApicPtr->Flags            = (UINT8)CpuApicIdOrderTable[LocalApicCounter].Flags;
      LocalApicPtr->ApicId           = (UINT8)CpuApicIdOrderTable[LocalApicCounter].ApicId;
      LocalApicPtr->AcpiProcessorUid = (UINT8)CpuApicIdOrderTable[LocalApicCounter].AcpiProcessorUid;
    } else {
      LocalApicPtr->Flags            = 0;
      LocalApicPtr->ApicId           = 0xFF;
      LocalApicPtr->AcpiProcessorUid = (UINT8)0xFF;
      Status = EFI_UNSUPPORTED;
    }
  } else if(Type == EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC) {
    if(mX2ApicEnabled) {
      LocalX2ApicPtr->Flags            = (UINT8)CpuApicIdOrderTable[LocalApicCounter].Flags;
      LocalX2ApicPtr->X2ApicId         = CpuApicIdOrderTable[LocalApicCounter].ApicId;
      LocalX2ApicPtr->AcpiProcessorUid = CpuApicIdOrderTable[LocalApicCounter].AcpiProcessorUid;
    } else {
      LocalX2ApicPtr->Flags            = 0;
      LocalX2ApicPtr->X2ApicId         = (UINT32)-1;
      LocalX2ApicPtr->AcpiProcessorUid = (UINT32)-1;
      Status = EFI_UNSUPPORTED;
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;

}

/**
  Sort CpuApicIdOrderTable based on the following rules:
  1.Make sure BSP is the first entry.
  2.Big core first, then small core.

  @param[in] CpuApicIdOrderTable      Pointer to EFI_CPU_ID_ORDER_MAP
  @param[in] Count                    Number to EFI_CPU_ID_ORDER_MAP
  @param[in] BspIndex                 BSP index
**/
VOID
SortApicIdOrderTable (
  IN  EFI_CPU_ID_ORDER_MAP  *CpuApicIdOrderTable,
  IN  UINTN                 Count,
  IN  UINTN                 BspIndex
  )
{
  UINTN                 Index;
  UINTN                 SubIndex;
  EFI_CPU_ID_ORDER_MAP  SortBuffer;

  //
  // Put BSP at the first entry.
  //
  if (BspIndex != 0) {
    CopyMem (&SortBuffer, &CpuApicIdOrderTable[BspIndex], sizeof (EFI_CPU_ID_ORDER_MAP));
    CopyMem (&CpuApicIdOrderTable[1], CpuApicIdOrderTable, (BspIndex) * sizeof (EFI_CPU_ID_ORDER_MAP));
    CopyMem (CpuApicIdOrderTable, &SortBuffer, sizeof (EFI_CPU_ID_ORDER_MAP));
  }

  //
  // If there are more than 2 cores, perform insertion sort for rest cores except the bsp in first entry
  // to move big cores in front of small cores.
  // Also the original order based on the MpService index inside big cores and small cores are retained.
  //
  for (Index = 2; Index < Count; Index++) {
    if (CpuApicIdOrderTable[Index].CoreType == CPUID_CORE_TYPE_INTEL_ATOM) {
      continue;
    }

    CopyMem (&SortBuffer, &CpuApicIdOrderTable[Index], sizeof (EFI_CPU_ID_ORDER_MAP));

    for (SubIndex = Index - 1; SubIndex >= 1; SubIndex--) {
      if (CpuApicIdOrderTable[SubIndex].CoreType == CPUID_CORE_TYPE_INTEL_ATOM) {
        CopyMem (&CpuApicIdOrderTable[SubIndex + 1], &CpuApicIdOrderTable[SubIndex], sizeof (EFI_CPU_ID_ORDER_MAP));
      } else {
        //
        // Except the BSP, all cores in front of SubIndex must be big cores.
        //
        break;
      }
    }

    CopyMem (&CpuApicIdOrderTable[SubIndex + 1], &SortBuffer, sizeof (EFI_CPU_ID_ORDER_MAP));
  }
}

/**
  Get CPU core type.

  @param[in] CpuApicIdOrderTable         Point to a buffer which will be filled in Core type information.
**/
VOID
EFIAPI
CollectCpuCoreType (
  IN EFI_CPU_ID_ORDER_MAP  *CpuApicIdOrderTable
  )
{
  UINTN                                    ApNumber;
  EFI_STATUS                               Status;
  CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX  NativeModelIdAndCoreTypeEax;

  Status = mMpService->WhoAmI (
                         mMpService,
                         &ApNumber
                         );
  ASSERT_EFI_ERROR (Status);

  AsmCpuidEx (CPUID_HYBRID_INFORMATION, CPUID_HYBRID_INFORMATION_MAIN_LEAF, &NativeModelIdAndCoreTypeEax.Uint32, NULL, NULL, NULL);
  CpuApicIdOrderTable[ApNumber].CoreType = (UINT8)NativeModelIdAndCoreTypeEax.Bits.CoreType;
}

/**
  Collect all processors information and create a Cpu Apic Id table.

  @param[in]  CpuApicIdOrderTable       Buffer to store information of Cpu.
**/
EFI_STATUS
CreateCpuLocalApicInTable (
  IN EFI_CPU_ID_ORDER_MAP *CpuApicIdOrderTable
  )
{
  EFI_STATUS                                Status;
  EFI_PROCESSOR_INFORMATION                 ProcessorInfoBuffer;
  UINT32                                    Index;
  UINT32                                    CurrProcessor;
  EFI_CPU_ID_ORDER_MAP                      *CpuIdMapPtr;
  UINT32                                    Socket;
  UINT32                                    CpuidMaxInput;
  UINTN                                     BspIndex;

  Status = EFI_SUCCESS;

  AsmCpuid (CPUID_SIGNATURE, &CpuidMaxInput, NULL, NULL, NULL);
  if (CpuidMaxInput >= CPUID_HYBRID_INFORMATION) {
    CollectCpuCoreType (CpuApicIdOrderTable);
    mMpService->StartupAllAPs (
                  mMpService,                               // This
                  (EFI_AP_PROCEDURE) CollectCpuCoreType,    // Procedure
                  TRUE,                                     // SingleThread
                  NULL,                                     // WaitEvent
                  0,                                        // TimeoutInMicrosecsond
                  CpuApicIdOrderTable,                      // ProcedureArgument
                  NULL                                      // FailedCpuList
                  );
  }

  for (CurrProcessor = 0, Index = 0; CurrProcessor < mNumberOfCpus; CurrProcessor++, Index++) {
    Status = mMpService->GetProcessorInfo (
                           mMpService,
                           CurrProcessor,
                           &ProcessorInfoBuffer
                           );

    if ((ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) != 0) {
      BspIndex = Index;
    }

    CpuIdMapPtr = (EFI_CPU_ID_ORDER_MAP *) &CpuApicIdOrderTable[Index];
    if ((ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT) != 0) {
      CpuIdMapPtr->ApicId  = (UINT32)ProcessorInfoBuffer.ProcessorId;
      CpuIdMapPtr->Thread  = ProcessorInfoBuffer.Location.Thread;
      CpuIdMapPtr->Flags   = ((ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT) != 0);
      CpuIdMapPtr->SocketNum = ProcessorInfoBuffer.Location.Package;
    } else {  //not enabled
      CpuIdMapPtr->ApicId     = (UINT32)-1;
      CpuIdMapPtr->Thread     = (UINT32)-1;
      CpuIdMapPtr->Flags      = 0;
      CpuIdMapPtr->SocketNum  = (UINT32)-1;
    } //end if PROC ENABLE
  } //end for CurrentProcessor

  //
  // Get Bsp Apic Id
  //
  DEBUG ((DEBUG_INFO, "BspApicId - 0x%x\n", GetApicId ()));


  //
  // Fill in AcpiProcessorUid.
  //
  for (Socket = 0; Socket < FixedPcdGet32 (PcdMaxCpuSocketCount); Socket++) {
    for (CurrProcessor = 0, Index = 0; CurrProcessor < mNumberOfCpus; CurrProcessor++) {
      if (CpuApicIdOrderTable[CurrProcessor].SocketNum == Socket) {
        CpuApicIdOrderTable[CurrProcessor].AcpiProcessorUid = (CpuApicIdOrderTable[CurrProcessor].SocketNum << mNumOfBitShift) + Index;
        Index++;
      }
    }
  }

  SortApicIdOrderTable (CpuApicIdOrderTable, mNumberOfCpus, BspIndex);

  DEBUG ((DEBUG_INFO, "::ACPI::  APIC ID Order Table Init.   mNumOfBitShift = %x\n", mNumOfBitShift));
  DebugDisplayReOrderTable (CpuApicIdOrderTable);

  return Status;
}


/** Structure of a sub-structure of the ACPI header.

  This structure contains the type and length fields, which are common to every
  sub-structure of the ACPI tables. A pointer to any structure can be cast as this.
**/
typedef struct {
  UINT8 Type;
  UINT8 Length;
} STRUCTURE_HEADER;

STRUCTURE_HEADER mMadtStructureTable[] = {
  {EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC,          sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE)},
  {EFI_ACPI_6_5_IO_APIC,                       sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE)},
  {EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE,     sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE)},
  {EFI_ACPI_6_5_NON_MASKABLE_INTERRUPT_SOURCE, sizeof (EFI_ACPI_6_5_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE)},
  {EFI_ACPI_6_5_LOCAL_APIC_NMI,                sizeof (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE)},
  {EFI_ACPI_6_5_LOCAL_APIC_ADDRESS_OVERRIDE,   sizeof (EFI_ACPI_6_5_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE)},
  {EFI_ACPI_6_5_IO_SAPIC,                      sizeof (EFI_ACPI_6_5_IO_SAPIC_STRUCTURE)},
  {EFI_ACPI_6_5_LOCAL_SAPIC,                   sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_SAPIC_STRUCTURE)},
  {EFI_ACPI_6_5_PLATFORM_INTERRUPT_SOURCES,    sizeof (EFI_ACPI_6_5_PLATFORM_INTERRUPT_SOURCES_STRUCTURE)},
  {EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC,        sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE)},
  {EFI_ACPI_6_5_LOCAL_X2APIC_NMI,              sizeof (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE)}
};

/**
  Get the size of the ACPI table.

  This function calculates the size needed for the ACPI Table based on the number and
  size of the sub-structures that will compose it.

  @param[in]  TableSpecificHdrLength  Size of the table specific header, not the ACPI standard header size.
  @param[in]  Structures              Pointer to an array of sub-structure pointers.
  @param[in]  StructureCount          Number of structure pointers in the array.

  @return     Total size needed for the ACPI table.
**/
UINT32
GetTableSize (
  IN  UINTN                 TableSpecificHdrLength,
  IN  STRUCTURE_HEADER      **Structures,
  IN  UINTN                 StructureCount
  )
{
  UINT32  TableLength;
  UINT32  Index;

  //
  // Compute size of the ACPI table; header plus all structures needed.
  //
  TableLength = (UINT32) TableSpecificHdrLength;

  for (Index = 0; Index < StructureCount; Index++) {
    ASSERT (Structures[Index] != NULL);
    if (Structures[Index] == NULL) {
      return 0;
    }

    TableLength += Structures[Index]->Length;
  }

  return TableLength;
}

/**
  Allocate the ACPI Table.

  This function allocates space for the ACPI table based on the number and size of
  the sub-structures that will compose it.

  @param[in]  TableSpecificHdrLength  Size of the table specific header, not the ACPI standard header size.
  @param[in]  Structures  Pointer to an array of sub-structure pointers.
  @param[in]  StructureCount  Number of structure pointers in the array.
  @param[out] Table            Newly allocated ACPI Table pointer.

  @retval EFI_SUCCESS           Successfully allocated the Table.
  @retval EFI_OUT_OF_RESOURCES  Space for the Table could not be allocated.
**/
EFI_STATUS
AllocateTable (
  IN  UINTN                        TableSpecificHdrLength,
  IN  STRUCTURE_HEADER             **Structures,
  IN  UINTN                        StructureCount,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  EFI_STATUS  Status;
  UINT32      Size;
  EFI_ACPI_DESCRIPTION_HEADER *InternalTable;

  //
  // Get the size of the ACPI table and allocate memory.
  //
  Size = GetTableSize (TableSpecificHdrLength, Structures, StructureCount);
  InternalTable = (EFI_ACPI_DESCRIPTION_HEADER *) AllocatePool (Size);

  if (InternalTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "Failed to allocate %d bytes for ACPI Table\n",
      Size
      ));
  } else {
    Status = EFI_SUCCESS;
    DEBUG ((
      DEBUG_INFO,
      "Successfully allocated %d bytes for ACPI Table at 0x%p\n",
      Size,
      InternalTable
      ));
    *Table = InternalTable;
  }

  return Status;
}

/**
  Initialize the header.

  This function fills in the standard table header with correct values,
  except for the length and checksum fields, which are filled in later.

  @param[in,out]  Header        Pointer to the header structure.

  @retval EFI_SUCCESS           Successfully initialized the header.
  @retval EFI_INVALID_PARAMETER Pointer parameter was null.
**/
EFI_STATUS
InitializeHeader (
  IN OUT  EFI_ACPI_DESCRIPTION_HEADER *Header,
  IN      UINT32                      Signature,
  IN      UINT8                       Revision,
  IN      UINT32                      OemRevision
  )
{
  UINT64 AcpiTableOemId;

  if (Header == NULL) {
    DEBUG ((DEBUG_ERROR, "Header pointer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  Header->Signature  = Signature;
  Header->Length     = 0; // filled in by Build function
  Header->Revision   = Revision;
  Header->Checksum   = 0; // filled in by InstallAcpiTable

  CopyMem (
    (VOID *) &Header->OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (Header->OemId)
    );

  AcpiTableOemId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (
    (VOID *) &Header->OemTableId,
    (VOID *) &AcpiTableOemId,
    sizeof (Header->OemTableId)
    );

  Header->OemRevision     = OemRevision;
  Header->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  Header->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  return EFI_SUCCESS;
}

/**
  Initialize the MADT header.

  This function fills in the MADT's standard table header with correct values,
  except for the length and checksum fields, which are filled in later.

  @param[in,out]  MadtHeader    Pointer to the MADT header structure.

  @retval EFI_SUCCESS           Successfully initialized the MADT header.
  @retval EFI_INVALID_PARAMETER Pointer parameter was null.
**/
EFI_STATUS
InitializeMadtHeader (
  IN OUT EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *MadtHeader
  )
{
  EFI_STATUS Status;

  if (MadtHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "MADT header pointer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  Status = InitializeHeader (
             &MadtHeader->Header,
             EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
             EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MadtHeader->LocalApicAddress       = PcdGet32(PcdLocalApicAddress);
  MadtHeader->Flags                  = EFI_ACPI_6_5_PCAT_COMPAT;

  return EFI_SUCCESS;
}

/**
  Copy an ACPI sub-structure; MADT and SRAT supported

  This function validates the structure type and size of a sub-structure
  and returns a newly allocated copy of it.

  @param[in]  Header            Pointer to the header of the table.
  @param[in]  Structure         Pointer to the structure to copy.
  @param[in]  NewStructure      Newly allocated copy of the structure.

  @retval EFI_SUCCESS           Successfully copied the structure.
  @retval EFI_INVALID_PARAMETER Pointer parameter was null.
  @retval EFI_INVALID_PARAMETER Structure type was unknown.
  @retval EFI_INVALID_PARAMETER Structure length was wrong for its type.
  @retval EFI_UNSUPPORTED       Header passed in is not supported.
**/
EFI_STATUS
CopyStructure (
  IN  EFI_ACPI_DESCRIPTION_HEADER *Header,
  IN  STRUCTURE_HEADER *Structure,
  OUT STRUCTURE_HEADER **NewStructure
  )
{
  STRUCTURE_HEADER      *NewStructureInternal;
  STRUCTURE_HEADER      *StructureTable;
  UINTN                 TableNumEntries;
  BOOLEAN               EntryFound;
  UINT8                 Index;

  //
  // Initialize the number of table entries and the table based on the table header passed in.
  //
  if (Header->Signature == EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE) {
    TableNumEntries = sizeof (mMadtStructureTable) / sizeof (STRUCTURE_HEADER);
    StructureTable = mMadtStructureTable;
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Check the incoming structure against the table of supported structures.
  //
  EntryFound = FALSE;
  for (Index = 0; Index < TableNumEntries; Index++) {
    if (Structure->Type == StructureTable[Index].Type) {
      if (Structure->Length == StructureTable[Index].Length) {
        EntryFound = TRUE;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "Invalid length for structure type %d: expected %d, actually %d\n",
          Structure->Type,
          StructureTable[Index].Length,
          Structure->Length
          ));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  //
  // If no entry in the table matches the structure type and length passed in
  // then return invalid parameter.
  //
  if (!EntryFound) {
    DEBUG ((
      DEBUG_ERROR,
      "Unknown structure type: %d\n",
      Structure->Type
      ));
    return EFI_INVALID_PARAMETER;
  }

  NewStructureInternal = (STRUCTURE_HEADER *) AllocatePool (Structure->Length);
  if (NewStructureInternal == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to allocate %d bytes for type %d structure\n",
      Structure->Length,
      Structure->Type
      ));
    return EFI_OUT_OF_RESOURCES;
  } else {
    DEBUG ((
      DEBUG_INFO,
      "Successfully allocated %d bytes for type %d structure at 0x%p\n",
      Structure->Length,
      Structure->Type,
      NewStructureInternal
      ));
  }

  CopyMem (
    (VOID *) NewStructureInternal,
    (VOID *) Structure,
    Structure->Length
    );

  *NewStructure = NewStructureInternal;
  return EFI_SUCCESS;
}

/**
  Build ACPI Table. MADT tables supported.

  This function builds the ACPI table from the header plus the list of sub-structures
  passed in. The table returned by this function is ready to be installed using
  the ACPI table protocol's InstallAcpiTable function, which copies it into
  ACPI memory. After that, the caller should free the memory returned by this
  function.

  @param[in]  AcpiHeader             Pointer to the header structure.
  @param[in]  TableSpecificHdrLength Size of the table specific header, not the ACPI standard header size.
  @param[in]  Structures             Pointer to an array of sub-structure pointers.
  @param[in]  StructureCount         Number of structure pointers in the array.
  @param[out] NewTable               Newly allocated and initialized pointer to the ACPI Table.

  @retval EFI_SUCCESS           Successfully built the ACPI table.
  @retval EFI_INVALID_PARAMETER Pointer parameter was null.
  @retval EFI_INVALID_PARAMETER Header parameter had the wrong signature.
  @retval EFI_OUT_OF_RESOURCES  Space for the ACPI Table could not be allocated.
**/
EFI_STATUS
BuildAcpiTable (
  IN  EFI_ACPI_DESCRIPTION_HEADER  *AcpiHeader,
  IN  UINTN                        TableSpecificHdrLength,
  IN  STRUCTURE_HEADER             **Structures,
  IN  UINTN                        StructureCount,
  OUT UINT8                        **NewTable
  )
{
  EFI_STATUS                  Status;
  EFI_ACPI_DESCRIPTION_HEADER *InternalTable;
  UINTN                       Index;
  UINT8                       *CurrPtr;
  UINT8                       *EndOfTablePtr;

  if (AcpiHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "AcpiHeader pointer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (AcpiHeader->Signature != EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE) {
    DEBUG ((
      DEBUG_ERROR,
      "MADT header signature is expected, actually 0x%08x\n",
      AcpiHeader->Signature
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Structures == NULL) {
    DEBUG ((DEBUG_ERROR, "Structure array pointer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < StructureCount; Index++) {
    if (Structures[Index] == NULL) {
      DEBUG ((DEBUG_ERROR, "Structure pointer %d is NULL\n", Index));
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Allocate the memory needed for the table.
  //
  Status = AllocateTable (
             TableSpecificHdrLength,
             Structures,
             StructureCount,
             &InternalTable
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Copy Header and patch in structure length, checksum is programmed later
  // after all structures are populated.
  //
  CopyMem (
    (VOID *) InternalTable,
    (VOID *) AcpiHeader,
    TableSpecificHdrLength
    );

  InternalTable->Length = GetTableSize (TableSpecificHdrLength, Structures, StructureCount);

  //
  // Copy all the sub structures to the table.
  //
  CurrPtr = ((UINT8 *) InternalTable) + TableSpecificHdrLength;
  EndOfTablePtr = ((UINT8 *) InternalTable) + InternalTable->Length;

  for (Index = 0; Index < StructureCount; Index++) {
    ASSERT (Structures[Index] != NULL);
    if (Structures[Index] == NULL) {
      break;
    }

    CopyMem (
      (VOID *) CurrPtr,
      (VOID *) Structures[Index],
      Structures[Index]->Length
      );

    CurrPtr += Structures[Index]->Length;
    ASSERT (CurrPtr <= EndOfTablePtr);
    if (CurrPtr > EndOfTablePtr) {
      break;
    }
  }

  //
  // Update the return pointer.
  //
  *NewTable = (UINT8 *) InternalTable;
  return EFI_SUCCESS;
}

/**
  Build from scratch and install the MADT.

  @retval EFI_SUCCESS           The MADT was installed successfully.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate required structures.
**/
EFI_STATUS
InstallMadtFromScratch (
  VOID
  )
{
  EFI_STATUS                                          Status;
  UINTN                                               Index;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *NewMadtTable;
  UINTN                                               TableHandle;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER MadtTableHeader;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE         ProcLocalApicStruct;
  EFI_ACPI_6_5_IO_APIC_STRUCTURE                      IoApicStruct;
  EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE    IntSrcOverrideStruct;
  EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE               LocalApciNmiStruct;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE       ProcLocalX2ApicStruct;
  EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE             LocalX2ApicNmiStruct;
  EFI_CPU_ID_ORDER_MAP                                *CpuApicIdOrderTable;
  STRUCTURE_HEADER                                    **MadtStructs;
  UINTN                                               MaxMadtStructCount;
  UINTN                                               MadtStructsIndex;
  UINT32                                              CurrentIoApicAddress = (UINT32)(PcdGet32(PcdPcIoApicAddressBase));
  UINT32                                              PcIoApicEnable;
  UINT32                                              PcIoApicMask;
  UINTN                                               PcIoApicIndex;

  MadtStructs = NULL;
  NewMadtTable = NULL;
  CpuApicIdOrderTable = NULL;
  MaxMadtStructCount = 0;

  CpuApicIdOrderTable = AllocateZeroPool (mNumberOfCpus * sizeof (EFI_CPU_ID_ORDER_MAP));
  if (CpuApicIdOrderTable == NULL) {
    DEBUG ((DEBUG_ERROR, "Could not allocate CpuApicIdOrderTable structure pointer array\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Call for Local APIC ID Reorder
  Status = CreateCpuLocalApicInTable (CpuApicIdOrderTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CreateCpuLocalApicInTable failed: %r\n", Status));
    goto Done;
  }

  MaxMadtStructCount = (UINT32) (
    mNumberOfCpus +  // processor local APIC structures
    mNumberOfCpus +  // processor local x2APIC structures
    1 + PcdGet8(PcdPcIoApicCount) +   // I/O APIC structures
    2 +              // interrupt source override structures
    1 +              // local APIC NMI structures
    1                // local x2APIC NMI structures
    );               // other structures are not used

  MadtStructs = (STRUCTURE_HEADER **) AllocateZeroPool (MaxMadtStructCount * sizeof (STRUCTURE_HEADER *));
  if (MadtStructs == NULL) {
    DEBUG ((DEBUG_ERROR, "Could not allocate MADT structure pointer array\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the next index into the structure pointer array. It is
  // incremented every time a structure of any type is copied to the array.
  //
  MadtStructsIndex = 0;

  //
  // Initialize MADT Header Structure
  //
  Status = InitializeMadtHeader (&MadtTableHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeMadtHeader failed: %r\n", Status));
    goto Done;
  }

  DEBUG ((DEBUG_INFO, "Number of CPUs detected = %d \n", mNumberOfCpus));

  //
  // Build Processor Local APIC Structures and Processor Local X2APIC Structures
  //
  ProcLocalApicStruct.Type = EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC;
  ProcLocalApicStruct.Length = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE);

  ProcLocalX2ApicStruct.Type = EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC;
  ProcLocalX2ApicStruct.Length = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE);
  ProcLocalX2ApicStruct.Reserved[0] = 0;
  ProcLocalX2ApicStruct.Reserved[1] = 0;

  for (Index = 0; Index < mNumberOfCpus; Index++) {
    //
    // If x2APIC mode is not enabled, and if it is possible to express the
    // APIC ID as a UINT8, use a processor local APIC structure. Otherwise,
    // use a processor local x2APIC structure.
    //
    if (!mX2ApicEnabled && CpuApicIdOrderTable[Index].ApicId < MAX_UINT8) {
      ProcLocalApicStruct.Flags            = (UINT8) CpuApicIdOrderTable[Index].Flags;
      ProcLocalApicStruct.ApicId           = (UINT8) CpuApicIdOrderTable[Index].ApicId;
      ProcLocalApicStruct.AcpiProcessorUid = (UINT8) CpuApicIdOrderTable[Index].AcpiProcessorUid;

      ASSERT (MadtStructsIndex < MaxMadtStructCount);
      Status = CopyStructure (
                 &MadtTableHeader.Header,
                 (STRUCTURE_HEADER *) &ProcLocalApicStruct,
                 &MadtStructs[MadtStructsIndex++]
                 );
    } else if (CpuApicIdOrderTable[Index].ApicId != 0xFFFFFFFF) {
      ProcLocalX2ApicStruct.Flags            = (UINT8) CpuApicIdOrderTable[Index].Flags;
      ProcLocalX2ApicStruct.X2ApicId         = CpuApicIdOrderTable[Index].ApicId;
      ProcLocalX2ApicStruct.AcpiProcessorUid = CpuApicIdOrderTable[Index].AcpiProcessorUid;

      ASSERT (MadtStructsIndex < MaxMadtStructCount);
      Status = CopyStructure (
                 &MadtTableHeader.Header,
                 (STRUCTURE_HEADER *) &ProcLocalX2ApicStruct,
                 &MadtStructs[MadtStructsIndex++]
                 );
    }
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "CopyMadtStructure (local APIC/x2APIC) failed: %r\n", Status));
      goto Done;
    }
  }

  //
  // Build I/O APIC Structures
  //
  IoApicStruct.Type = EFI_ACPI_6_5_IO_APIC;
  IoApicStruct.Length = sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE);
  IoApicStruct.Reserved = 0;

  PcIoApicEnable = PcdGet32 (PcdPcIoApicEnable);

  if (FixedPcdGet32 (PcdMaxCpuSocketCount) <= 4) {
    IoApicStruct.IoApicId                  = PcdGet8(PcdIoApicId);
    IoApicStruct.IoApicAddress             = PcdGet32(PcdIoApicAddress);
    IoApicStruct.GlobalSystemInterruptBase = 0;
    ASSERT (MadtStructsIndex < MaxMadtStructCount);
    Status = CopyStructure (
               &MadtTableHeader.Header,
               (STRUCTURE_HEADER *) &IoApicStruct,
               &MadtStructs[MadtStructsIndex++]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "CopyMadtStructure (I/O APIC) failed: %r\n", Status));
      goto Done;
    }
  }

  for (PcIoApicIndex = 0; PcIoApicIndex < PcdGet8(PcdPcIoApicCount); PcIoApicIndex++) {
    PcIoApicMask = (1 << PcIoApicIndex);
    if ((PcIoApicEnable & PcIoApicMask) == 0) {
      continue;
    }

    IoApicStruct.IoApicId                  = (UINT8)(PcdGet8(PcdPcIoApicIdBase) + PcIoApicIndex);
    IoApicStruct.IoApicAddress             = CurrentIoApicAddress;
    CurrentIoApicAddress                   = (CurrentIoApicAddress & 0xFFFF8000) + 0x8000;
    IoApicStruct.GlobalSystemInterruptBase = (UINT32)(24 + (PcIoApicIndex * 8));
    ASSERT (MadtStructsIndex < MaxMadtStructCount);
    Status = CopyStructure (
               &MadtTableHeader.Header,
               (STRUCTURE_HEADER *) &IoApicStruct,
               &MadtStructs[MadtStructsIndex++]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "CopyMadtStructure (I/O APIC) failed: %r\n", Status));
      goto Done;
    }
  }

  //
  // Build Interrupt Source Override Structures
  //
  IntSrcOverrideStruct.Type = EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE;
  IntSrcOverrideStruct.Length = sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE);

  //
  // IRQ0=>IRQ2 Interrupt Source Override Structure
  //
  IntSrcOverrideStruct.Bus = 0x0;                   // Bus - ISA
  IntSrcOverrideStruct.Source = 0x0;                // Source - IRQ0
  IntSrcOverrideStruct.GlobalSystemInterrupt = 0x2; // Global System Interrupt - IRQ2
  IntSrcOverrideStruct.Flags = 0x0;                 // Flags - Conforms to specifications of the bus

  ASSERT (MadtStructsIndex < MaxMadtStructCount);
  Status = CopyStructure (
             &MadtTableHeader.Header,
             (STRUCTURE_HEADER *) &IntSrcOverrideStruct,
             &MadtStructs[MadtStructsIndex++]
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CopyMadtStructure (IRQ2 source override) failed: %r\n", Status));
    goto Done;
  }

  //
  // IRQ9 (SCI Active High) Interrupt Source Override Structure
  //
  IntSrcOverrideStruct.Bus = 0x0;                   // Bus - ISA
  IntSrcOverrideStruct.Source = 0x9;                // Source - IRQ9
  IntSrcOverrideStruct.GlobalSystemInterrupt = 0x9; // Global System Interrupt - IRQ9
  IntSrcOverrideStruct.Flags = 0xD;                 // Flags - Level-tiggered, Active High

  ASSERT (MadtStructsIndex < MaxMadtStructCount);
  Status = CopyStructure (
             &MadtTableHeader.Header,
             (STRUCTURE_HEADER *) &IntSrcOverrideStruct,
             &MadtStructs[MadtStructsIndex++]
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CopyMadtStructure (IRQ9 source override) failed: %r\n", Status));
    goto Done;
  }

  //
  // Build Local APIC NMI Structures
  //
  if (!mX2ApicEnabled) {
    LocalApciNmiStruct.Type   = EFI_ACPI_6_5_LOCAL_APIC_NMI;
    LocalApciNmiStruct.Length = sizeof (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE);
    LocalApciNmiStruct.AcpiProcessorUid = 0xFF;      // Applies to all processors
    LocalApciNmiStruct.Flags            = 0x0005;    // Flags - Edge-tiggered, Active High
    LocalApciNmiStruct.LocalApicLint    = 0x1;

    ASSERT (MadtStructsIndex < MaxMadtStructCount);
    Status = CopyStructure (
              &MadtTableHeader.Header,
              (STRUCTURE_HEADER *) &LocalApciNmiStruct,
              &MadtStructs[MadtStructsIndex++]
              );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "CopyMadtStructure (APIC NMI) failed: %r\n", Status));
      goto Done;
    }
  }

  //
  // Build Local x2APIC NMI Structure
  //
  if (mX2ApicEnabled) {
    LocalX2ApicNmiStruct.Type   = EFI_ACPI_6_5_LOCAL_X2APIC_NMI;
    LocalX2ApicNmiStruct.Length = sizeof (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE);
    LocalX2ApicNmiStruct.Flags  = 0x000D;                // Flags - Level-tiggered, Active High
    LocalX2ApicNmiStruct.AcpiProcessorUid = 0xFFFFFFFF;  // Applies to all processors
    LocalX2ApicNmiStruct.LocalX2ApicLint  = 0x01;
    LocalX2ApicNmiStruct.Reserved[0] = 0x00;
    LocalX2ApicNmiStruct.Reserved[1] = 0x00;
    LocalX2ApicNmiStruct.Reserved[2] = 0x00;

    ASSERT (MadtStructsIndex < MaxMadtStructCount);
    Status = CopyStructure (
               &MadtTableHeader.Header,
               (STRUCTURE_HEADER *) &LocalX2ApicNmiStruct,
               &MadtStructs[MadtStructsIndex++]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "CopyMadtStructure (x2APIC NMI) failed: %r\n", Status));
      goto Done;
    }
  }

  //
  // Build Madt Structure from the Madt Header and collection of pointers in MadtStructs[]
  //
  Status = BuildAcpiTable (
            (EFI_ACPI_DESCRIPTION_HEADER *) &MadtTableHeader,
            sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER),
            MadtStructs,
            MadtStructsIndex,
            (UINT8 **)&NewMadtTable
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildAcpiTable failed: %r\n", Status));
    goto Done;
  }

  //
  // Publish Madt Structure to ACPI
  //
  Status = mAcpiTable->InstallAcpiTable (
                         mAcpiTable,
                         NewMadtTable,
                         NewMadtTable->Header.Length,
                         &TableHandle
                         );

Done:
  //
  // Free memory
  //
  if (MadtStructs != NULL) {
    for (MadtStructsIndex = 0; MadtStructsIndex < MaxMadtStructCount; MadtStructsIndex++) {
      if (MadtStructs[MadtStructsIndex] != NULL) {
        FreePool (MadtStructs[MadtStructsIndex]);
      }
    }
    FreePool (MadtStructs);
  }

  if (NewMadtTable != NULL) {
    FreePool (NewMadtTable);
  }

  if (CpuApicIdOrderTable != NULL) {
    FreePool (CpuApicIdOrderTable);
  }

  return Status;
}

EFI_STATUS
InstallMcfgFromScratch (
  VOID
  )
{
  EFI_STATUS                                                                            Status;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER                        *McfgTable;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *Segment;
  UINTN                                                                                 Index;
  UINTN                                                                                 SegmentCount;
  PCI_SEGMENT_INFO                                                                      *PciSegmentInfo;
  UINTN                                                                                 TableHandle;

  PciSegmentInfo = GetPciSegmentInfo (&SegmentCount);

  McfgTable = AllocateZeroPool (
                sizeof (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER) +
                sizeof (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE) * SegmentCount
                );
  if (McfgTable == NULL) {
    DEBUG ((DEBUG_ERROR, "Could not allocate MCFG structure\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitializeHeader (
             &McfgTable->Header,
             EFI_ACPI_6_5_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
             EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
             FixedPcdGet32 (PcdAcpiDefaultOemRevision)
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Set MCFG table "Length" field based on the number of PCIe segments enumerated so far
  //
  McfgTable->Header.Length = (UINT32)(sizeof (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER) +
                                      sizeof (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE) * SegmentCount);

  Segment = (VOID *)(McfgTable + 1);

  for (Index = 0; Index < SegmentCount; Index++) {
    Segment[Index].PciSegmentGroupNumber  = PciSegmentInfo[Index].SegmentNumber;
    Segment[Index].BaseAddress    = PciSegmentInfo[Index].BaseAddress;
    Segment[Index].StartBusNumber = PciSegmentInfo[Index].StartBusNumber;
    Segment[Index].EndBusNumber   = PciSegmentInfo[Index].EndBusNumber;
  }

  //
  // Publish Mcfg Structure to ACPI
  //
  Status = mAcpiTable->InstallAcpiTable (
                         mAcpiTable,
                         McfgTable,
                         McfgTable->Header.Length,
                         &TableHandle
                         );
Done:
  FreePool (McfgTable);
  return Status;
}

/**
  This function will update any runtime platform specific information.
  This currently includes:
    Setting OEM table values, ID, table ID, creator ID and creator revision.
    Enabling the proper processor entries in the APIC tables
  It also indicates with which ACPI table version the table belongs.

  @param[in] Table        The table to update
  @param[in] Version      Where to install this table

  @retval EFI_SUCCESS     Updated tables commplete.
**/
EFI_STATUS
PlatformUpdateTables (
  IN OUT EFI_ACPI_COMMON_HEADER       *Table,
  IN OUT EFI_ACPI_TABLE_VERSION       *Version
  )
{
  EFI_ACPI_DESCRIPTION_HEADER                      *TableHeader;
  UINT8                                            *TempOemId;
  UINT64                                           TempOemTableId;
  EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE        *FadtHeader;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER *HpetTable;
  UINT32                                           HpetBaseAddress;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_BLOCK_ID     HpetBlockId;
  UINT32                                           HpetCapabilitiesData;
  HPET_GENERAL_CAPABILITIES_ID_REGISTER            HpetCapabilities;

  TableHeader             = NULL;

  //
  // By default, a table belongs in all ACPI table versions published.
  // Some tables will override this because they have different versions of the table.
  //
  TableHeader = (EFI_ACPI_DESCRIPTION_HEADER *) Table;

  //
  // Update the OEM and creator information for every table except FACS.
  //
  if (Table->Signature != EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) {
    TempOemId = (UINT8 *)PcdGetPtr(PcdAcpiDefaultOemId);
    CopyMem (&TableHeader->OemId, TempOemId, 6);

    //
    // Skip OEM table ID and creator information for DSDT, SSDT and PSDT tables, since these are
    // created by an ASL compiler and the creator information is useful.
    //
    if (Table->Signature != EFI_ACPI_6_5_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE &&
        Table->Signature != EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE &&
        Table->Signature != EFI_ACPI_6_5_PERSISTENT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
        ) {
      TempOemTableId = PcdGet64(PcdAcpiDefaultOemTableId);
      CopyMem (&TableHeader->OemTableId, &TempOemTableId, 8);

      //
      // Update the creator ID
      //
      TableHeader->CreatorId = PcdGet32(PcdAcpiDefaultCreatorId);

      //
      // Update the creator revision
      //
      TableHeader->CreatorRevision = PcdGet32(PcdAcpiDefaultCreatorRevision);

      //
      // Update the oem revision
      //
      TableHeader->OemRevision = PcdGet32(PcdAcpiDefaultOemRevision);
    }
  }


  //
  // By default, a table belongs in all ACPI table versions published.
  // Some tables will override this because they have different versions of the table.
  //
  *Version = EFI_ACPI_TABLE_VERSION_1_0B | EFI_ACPI_TABLE_VERSION_2_0 | EFI_ACPI_TABLE_VERSION_3_0;

  //
  // Update the various table types with the necessary updates
  //
  switch (Table->Signature) {

  case EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:
    ASSERT(FALSE);
    break;

  case EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:
    FadtHeader = (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE *) Table;

    FadtHeader->Header.Revision                   = PcdGet8 (PcdFadtMajorVersion);
    FadtHeader->PreferredPmProfile                = PcdGet8 (PcdFadtPreferredPmProfile);
    FadtHeader->IaPcBootArch                      = PcdGet16 (PcdFadtIaPcBootArch);
    FadtHeader->Flags                             = PcdGet32 (PcdFadtFlags);
    FadtHeader->SmiCmd                            = PcdGet32 (PcdFadtSmiCmd);
    FadtHeader->AcpiEnable                        = PcdGet8 (PcdAcpiEnableSwSmi);
    FadtHeader->AcpiDisable                       = PcdGet8 (PcdAcpiDisableSwSmi);
    FadtHeader->Pm1aEvtBlk                        = PcdGet16 (PcdAcpiPm1AEventBlockAddress);
    FadtHeader->Pm1bEvtBlk                        = PcdGet16 (PcdAcpiPm1BEventBlockAddress);
    FadtHeader->Pm1aCntBlk                        = PcdGet16 (PcdAcpiPm1AControlBlockAddress);
    FadtHeader->Pm1bCntBlk                        = PcdGet16 (PcdAcpiPm1BControlBlockAddress);
    FadtHeader->Pm2CntBlk                         = PcdGet16 (PcdAcpiPm2ControlBlockAddress);
    FadtHeader->PmTmrBlk                          = PcdGet16 (PcdAcpiPmTimerBlockAddress);
    FadtHeader->Gpe0Blk                           = PcdGet16 (PcdAcpiGpe0BlockAddress);
    FadtHeader->Gpe0BlkLen                        = PcdGet8 (PcdAcpiGpe0BlockLength);
    FadtHeader->Gpe1Blk                           = PcdGet16 (PcdAcpiGpe1BlockAddress);
    FadtHeader->Gpe1BlkLen                        = PcdGet8 (PcdAcpiGpe1BlockLength);
    FadtHeader->Gpe1Base                          = PcdGet8 (PcdAcpiGpe1Base);
    FadtHeader->MinorVersion                      = PcdGet8 (PcdFadtMinorVersion);

    FadtHeader->XPm1aEvtBlk.Address               = PcdGet16 (PcdAcpiPm1AEventBlockAddress);
    FadtHeader->XPm1bEvtBlk.Address               = PcdGet16 (PcdAcpiPm1BEventBlockAddress);
    FadtHeader->XPm1aCntBlk.Address               = PcdGet16 (PcdAcpiPm1AControlBlockAddress);
    FadtHeader->XPm1bCntBlk.Address               = PcdGet16 (PcdAcpiPm1BControlBlockAddress);
    FadtHeader->XPm2CntBlk.Address                = PcdGet16 (PcdAcpiPm2ControlBlockAddress);
    FadtHeader->XPmTmrBlk.Address                 = PcdGet16 (PcdAcpiPmTimerBlockAddress);
    FadtHeader->XGpe0Blk.Address                  = PcdGet16 (PcdAcpiGpe0BlockAddress);
    FadtHeader->XGpe1Blk.Address                  = PcdGet16 (PcdAcpiGpe1BlockAddress);

    FadtHeader->ResetReg.AccessSize               = PcdGet8 (PcdAcpiResetRegisterAccessSize);
    FadtHeader->XPm1aEvtBlk.AccessSize            = PcdGet8 (PcdAcpiXPm1aEvtBlkAccessSize);
    FadtHeader->XPm1bEvtBlk.AccessSize            = PcdGet8 (PcdAcpiXPm1bEvtBlkAccessSize);
    FadtHeader->XPm1aCntBlk.AccessSize            = PcdGet8 (PcdAcpiXPm1aCntBlkAccessSize);
    FadtHeader->XPm1bCntBlk.AccessSize            = PcdGet8 (PcdAcpiXPm1bCntBlkAccessSize);
    FadtHeader->XPm2CntBlk.AccessSize             = PcdGet8 (PcdAcpiXPm2CntBlkAccessSize);
    FadtHeader->XPmTmrBlk.AccessSize              = PcdGet8 (PcdAcpiXPmTmrBlkAccessSize);
    FadtHeader->XGpe0Blk.AccessSize               = PcdGet8 (PcdAcpiXGpe0BlkAccessSize);
    FadtHeader->XGpe1Blk.AccessSize               = PcdGet8 (PcdAcpiXGpe1BlkAccessSize);
    FadtHeader->XGpe1Blk.RegisterBitWidth         = PcdGet8 (PcdAcpiXGpe1BlkRegisterBitWidth);

    FadtHeader->SleepControlReg.AddressSpaceId    = PcdGet8 (PcdAcpiSleepControlRegisterAddressSpaceId);
    FadtHeader->SleepControlReg.RegisterBitWidth  = PcdGet8 (PcdAcpiSleepControlRegisterBitWidth);
    FadtHeader->SleepControlReg.RegisterBitOffset = PcdGet8 (PcdAcpiSleepControlRegisterBitOffset);
    FadtHeader->SleepControlReg.AccessSize        = PcdGet8 (PcdAcpiSleepControlRegisterAccessSize);
    FadtHeader->SleepControlReg.Address           = PcdGet64 (PcdAcpiSleepControlRegisterAddress);
    FadtHeader->SleepStatusReg.AddressSpaceId     = PcdGet8 (PcdAcpiSleepStatusRegisterAddressSpaceId);
    FadtHeader->SleepStatusReg.RegisterBitWidth   = PcdGet8 (PcdAcpiSleepStatusRegisterBitWidth);
    FadtHeader->SleepStatusReg.RegisterBitOffset  = PcdGet8 (PcdAcpiSleepStatusRegisterBitOffset);
    FadtHeader->SleepStatusReg.AccessSize         = PcdGet8 (PcdAcpiSleepStatusRegisterAccessSize);
    FadtHeader->SleepStatusReg.Address            = PcdGet64 (PcdAcpiSleepStatusRegisterAddress);

    FadtHeader->S4BiosReq                         = PcdGet8 (PcdAcpiS4BiosReq);
    FadtHeader->XPm1aEvtBlk.Address               = PcdGet16 (PcdAcpiPm1AEventBlockAddress);
    FadtHeader->XPm1bEvtBlk.Address               = PcdGet16 (PcdAcpiPm1BEventBlockAddress);

    FadtHeader->DutyOffset                        = PcdGet8 (PcdFadtDutyOffset);
    FadtHeader->DutyWidth                         = PcdGet8 (PcdFadtDutyWidth);

    DEBUG ((DEBUG_INFO, "ACPI FADT table @ address 0x%x\n", Table));
    DEBUG ((DEBUG_INFO, "  IaPcBootArch 0x%x\n", FadtHeader->IaPcBootArch));
    DEBUG ((DEBUG_INFO, "  Flags 0x%x\n", FadtHeader->Flags));
    break;

  case EFI_ACPI_6_5_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE:
    HpetTable = (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER *)Table;
    HpetBaseAddress = PcdGet32 (PcdHpetBaseAddress);
    HpetTable->BaseAddressLower32Bit.Address = HpetBaseAddress;
    HpetTable->BaseAddressLower32Bit.RegisterBitWidth = 0;
    HpetCapabilitiesData     = MmioRead32 (HpetBaseAddress + HPET_GENERAL_CAPABILITIES_ID_OFFSET);
    HpetCapabilities.Uint64  = HpetCapabilitiesData;
    HpetCapabilitiesData     = MmioRead32 (HpetBaseAddress + HPET_GENERAL_CAPABILITIES_ID_OFFSET + 4);
    HpetCapabilities.Uint64 |= LShiftU64 (HpetCapabilitiesData, 32);
    HpetBlockId.Bits.Revision       = HpetCapabilities.Bits.Revision;
    HpetBlockId.Bits.NumberOfTimers = HpetCapabilities.Bits.NumberOfTimers;
    HpetBlockId.Bits.CounterSize    = HpetCapabilities.Bits.CounterSize;
    HpetBlockId.Bits.Reserved       = 0;
    HpetBlockId.Bits.LegacyRoute    = HpetCapabilities.Bits.LegacyRoute;
    HpetBlockId.Bits.VendorId       = HpetCapabilities.Bits.VendorId;
    HpetTable->EventTimerBlockId    = HpetBlockId.Uint32;
    HpetTable->MainCounterMinimumClockTickInPeriodicMode = (UINT16)HpetCapabilities.Bits.CounterClockPeriod;
    DEBUG ((DEBUG_INFO, "ACPI HPET table @ address 0x%x\n", Table));
    DEBUG ((DEBUG_INFO, "  HPET base 0x%x\n", PcdGet32 (PcdHpetBaseAddress)));
    break;

  case EFI_ACPI_6_5_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE:
    ASSERT (FALSE);
    break;

  default:
    break;
  }
  return EFI_SUCCESS;
}

/**
  Function prototype for GetAcpiTableCount/CalculateAcpiTableCrc.

  @param[in] Table        The pointer to ACPI table.
  @param[in] TableIndex   The ACPI table index.
  @param[in] Context      The pointer to UINTN for GetAcpiTableCount.
                          The pointer to UINT32 array for CalculateAcpiTableCrc.
**/
typedef
VOID
(EFIAPI *ACPI_TABLE_CALLBACK)(
  IN  EFI_ACPI_COMMON_HEADER  *Table,
  IN  UINTN                   TableIndex,
  IN  VOID                    *Context
  );

/**
  Enumerate all ACPI tables in RSDT/XSDT.

  @param[in] Sdt                ACPI XSDT/RSDT.
  @param[in] TablePointerSize   Size of table pointer:
                                4(RSDT) or 8(XSDT).
  @param[in] CallbackFunction   The pointer to GetAcpiTableCount/CalculateAcpiTableCrc.
  @param[in] Context            The pointer to UINTN for GetAcpiTableCount.
                                The pointer to UINT32 array for CalculateAcpiTableCrc.
**/
VOID
EnumerateAllAcpiTables (
  IN  EFI_ACPI_DESCRIPTION_HEADER  *Sdt,
  IN  UINTN                        TablePointerSize,
  IN  ACPI_TABLE_CALLBACK          CallbackFunction,
  IN  VOID                         *Context
  )
{
  UINTN                                      Index;
  UINTN                                      TableIndex;
  UINTN                                      EntryCount;
  UINT64                                     EntryPtr;
  UINTN                                      BasePtr;
  EFI_ACPI_COMMON_HEADER                     *Table;
  EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE  *FadtPtr;

  Index      = 0;
  TableIndex = 0;
  EntryCount = (Sdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / TablePointerSize;
  EntryPtr   = 0;
  BasePtr    = (UINTN)(Sdt + 1);
  Table      = NULL;
  FadtPtr    = NULL;

  if (Sdt == NULL) {
    ASSERT (Sdt != NULL);
    return;
  }

  for (Index = 0; Index < EntryCount; Index++) {
    EntryPtr = 0;
    Table    = NULL;
    CopyMem (&EntryPtr, (VOID *)(BasePtr + Index * TablePointerSize), TablePointerSize);
    Table = (EFI_ACPI_COMMON_HEADER *)((UINTN)(EntryPtr));
    if (Table != NULL) {
      CallbackFunction (Table, TableIndex++, Context);
    }

    if (Table->Signature == EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      FadtPtr = (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE *)Table;
      if (FadtPtr->Header.Revision < EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION) {
        //
        // Locate FACS/DSDT in FADT
        //
        CallbackFunction ((EFI_ACPI_COMMON_HEADER *)(UINTN)FadtPtr->FirmwareCtrl, TableIndex++, Context);
        CallbackFunction ((EFI_ACPI_COMMON_HEADER *)(UINTN)FadtPtr->Dsdt, TableIndex++, Context);
      } else {
        //
        // Locate FACS in FADT
        //
        if (FadtPtr->XFirmwareCtrl != 0) {
          CallbackFunction ((EFI_ACPI_COMMON_HEADER *)(UINTN)FadtPtr->XFirmwareCtrl, TableIndex++, Context);
        } else {
          CallbackFunction ((EFI_ACPI_COMMON_HEADER *)(UINTN)FadtPtr->FirmwareCtrl, TableIndex++, Context);
        }

        //
        // Locate DSDT in FADT
        //
        if (FadtPtr->XDsdt != 0) {
          CallbackFunction ((EFI_ACPI_COMMON_HEADER *)(UINTN)FadtPtr->XDsdt, TableIndex++, Context);
        } else {
          CallbackFunction ((EFI_ACPI_COMMON_HEADER *)(UINTN)FadtPtr->Dsdt, TableIndex++, Context);
        }
      }
    }
  }
}

/**
  Count the number of ACPI tables.

  @param[in] Table        The pointer to ACPI table.
  @param[in] TableIndex   The ACPI table index.
  @param[in] Context      The pointer to UINTN.
**/
VOID
EFIAPI
GetAcpiTableCount (
  IN  EFI_ACPI_COMMON_HEADER  *Table,
  IN  UINTN                   TableIndex,
  IN  VOID                    *Context
  )
{
  UINTN  *TableCount;

  TableCount = (UINTN *)Context;

  if (Table == NULL) {
    ASSERT (Table != NULL);
    return;
  }

  (*TableCount)++;
}

/**
  Calculate CRC based on each offset in the ACPI table.

  @param[in] Table        The pointer to ACPI table.
  @param[in] TableIndex   The ACPI table index.
  @param[in] Context      The pointer to UINT32 array.
**/
VOID
EFIAPI
CalculateAcpiTableCrc (
  IN  EFI_ACPI_COMMON_HEADER  *Table,
  IN  UINTN                   TableIndex,
  IN  VOID                    *Context
  )
{
  UINT32  *TableCrcRecord;

  TableCrcRecord = (UINT32 *)Context;

  if (Table == NULL) {
    ASSERT (Table != NULL);
    return;
  }

  //
  // Calculate CRC value.
  //
  if (Table->Signature == EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) {
    //
    // Zero HardwareSignature field before Calculating FACS CRC
    //
    ((EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE *)Table)->HardwareSignature = 0;
  }

  gBS->CalculateCrc32 ((UINT8 *)Table, (UINTN)Table->Length, &TableCrcRecord[TableIndex]);
}

/**
  This function calculates CRC based on each ACPI table.
  It also calculates CRC and provides as HardwareSignature field in FACS.
**/
VOID
IsAcpiTableChange (
  VOID
  )
{
  EFI_STATUS                                    Status;
  BOOLEAN                                       IsRsdt;
  UINTN                                         AcpiTableCount;
  UINT32                                        *TableCrcRecord;
  EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE  *FacsPtr;

  IsRsdt         = FALSE;
  AcpiTableCount = 0;
  TableCrcRecord = NULL;
  Rsdp           = NULL;
  Rsdt           = NULL;
  Xsdt           = NULL;
  FacsPtr        = NULL;

  DEBUG ((DEBUG_INFO, "%a() - Start\n", __func__));

  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, (VOID **)&Rsdp);
  if (EFI_ERROR (Status) || (Rsdp == NULL)) {
    return;
  }

  Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->RsdtAddress;
  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->XsdtAddress;
  if (Xsdt == NULL) {
    if (Rsdt != NULL) {
      IsRsdt = TRUE;
    } else {
      return;
    }
  }

  FacsPtr = (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE *)EfiLocateFirstAcpiTable (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE);
  if (FacsPtr == NULL) {
    return;
  }

  //
  // Count the ACPI tables found by RSDT/XSDT and FADT.
  //
  if (IsRsdt) {
    EnumerateAllAcpiTables (Rsdt, sizeof (UINT32), GetAcpiTableCount, (VOID *)&AcpiTableCount);
  } else {
    EnumerateAllAcpiTables (Xsdt, sizeof (UINT64), GetAcpiTableCount, (VOID *)&AcpiTableCount);
  }

  //
  // Allocate memory for founded ACPI tables.
  //
  TableCrcRecord = AllocateZeroPool (sizeof (UINT32) * AcpiTableCount);
  if (TableCrcRecord == NULL) {
    return;
  }

  //
  // Calculate CRC for each ACPI table and set record.
  //
  if (IsRsdt) {
    EnumerateAllAcpiTables (Rsdt, sizeof (UINT32), CalculateAcpiTableCrc, (VOID *)TableCrcRecord);
  } else {
    EnumerateAllAcpiTables (Xsdt, sizeof (UINT64), CalculateAcpiTableCrc, (VOID *)TableCrcRecord);
  }

  //
  // Calculate and set HardwareSignature data.
  //
  Status = gBS->CalculateCrc32 ((UINT8 *)TableCrcRecord, AcpiTableCount, &(FacsPtr->HardwareSignature));
  DEBUG ((DEBUG_INFO, "HardwareSignature = %x and Status = %r\n", FacsPtr->HardwareSignature, Status));

  FreePool (TableCrcRecord);
  DEBUG ((DEBUG_INFO, "%a() - End\n", __func__));
}

VOID
UpdateLocalTable (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_ACPI_COMMON_HEADER        *CurrentTable;
  EFI_ACPI_TABLE_VERSION        Version;
  UINTN                         TableHandle;
  UINTN                         Index;

  for (Index = 0; Index < sizeof(mLocalTable)/sizeof(mLocalTable[0]); Index++) {
    CurrentTable = mLocalTable[Index];

    PlatformUpdateTables (CurrentTable, &Version);

    TableHandle = 0;

    if (Version != EFI_ACPI_TABLE_VERSION_NONE) {
      Status = mAcpiTable->InstallAcpiTable (
                             mAcpiTable,
                             CurrentTable,
                             CurrentTable->Length,
                             &TableHandle
                             );
      ASSERT_EFI_ERROR (Status);
    }
  }
}

VOID
EFIAPI
AcpiEndOfDxeEvent (
  EFI_EVENT           Event,
  VOID                *ParentImageHandle
  )
{
  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  //
  // Calculate Hardware Signature value based on current platform configurations
  //
  IsAcpiTableChange ();
}

/**
  ACPI Platform driver installation function.

  @param[in] ImageHandle     Handle for this drivers loaded image protocol.
  @param[in] SystemTable     EFI system table.

  @retval EFI_SUCCESS        The driver installed without error.
  @retval EFI_ABORTED        The driver encountered an error and could not complete installation of
                             the ACPI tables.

**/
EFI_STATUS
EFIAPI
InstallAcpiPlatform (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_EVENT                     EndOfDxeEvent;

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&mMpService);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&mAcpiTable);
  ASSERT_EFI_ERROR (Status);

  //
  // Create an End of DXE event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiEndOfDxeEvent,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Determine the number of processors
  //
  mMpService->GetNumberOfProcessors (
                mMpService,
                &mNumberOfCpus,
                &mNumberOfEnabledCPUs
                );

  DEBUG ((DEBUG_INFO, "mNumberOfCpus - %d\n", mNumberOfCpus));
  DEBUG ((DEBUG_INFO, "mNumberOfEnabledCPUs - %d\n", mNumberOfEnabledCPUs));

  if (LOCAL_APIC_MODE_X2APIC == GetApicMode ()) {
    mX2ApicEnabled = TRUE;
  }

  DEBUG ((DEBUG_INFO, "mX2ApicEnabled - 0x%x\n", mX2ApicEnabled));

  // support up to 64 threads/socket
  AsmCpuidEx (CPUID_EXTENDED_TOPOLOGY, 1, &mNumOfBitShift, NULL, NULL, NULL);
  mNumOfBitShift &= 0x1F;
  DEBUG ((DEBUG_INFO, "mNumOfBitShift - 0x%x\n", mNumOfBitShift));

  UpdateLocalTable ();

  InstallMadtFromScratch ();
  InstallMcfgFromScratch ();

  return EFI_SUCCESS;
}
