/** @file
*  This file is an ACPI driver for the Qemu SBSA platform.
*
*  Copyright (c) 2020-2024, Linaro Ltd. All rights reserved.
*  Copyright (c) 2020-2021, Ampere Computing LLC. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/AcpiAml.h>
#include <IndustryStandard/ArmCache.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <IndustryStandard/SbsaQemuAcpi.h>
#include <IndustryStandard/SbsaQemuPlatformVersion.h>
#include <Library/ArmLib/AArch64/AArch64Lib.h>
#include <Library/AcpiLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/HardwareInfoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/AcpiTable.h>
#include "SbsaQemuAcpiDxe.h"

#pragma pack(1)

static UINTN  GicItsBase;

#pragma pack ()

static UINTN  mCpuId;
static UINTN  mCacheId;

/*
 * A Function to Compute the ACPI Table Checksum
 */
VOID
AcpiPlatformChecksum (
  IN UINT8  *Buffer,
  IN UINTN  Size
  )
{
  UINTN  ChecksumOffset;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  // Set checksum field to 0 since it is used as part of the calculation
  Buffer[ChecksumOffset] = 0;

  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);
}

/*
 * A function that add the IORT ACPI table.
  IN EFI_ACPI_COMMON_HEADER    *CurrentTable
 */
EFI_STATUS
AddIortTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS            Status;
  UINTN                 TableHandle;
  UINT32                TableSize;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINT8                 *New;

  // Initialize IORT ACPI Header
  EFI_ACPI_6_0_IO_REMAPPING_TABLE  Header = {
    SBSAQEMU_ACPI_HEADER (
      EFI_ACPI_6_0_IO_REMAPPING_TABLE_SIGNATURE,
      SBSA_IO_REMAPPING_STRUCTURE,
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_06
      ),
    3,
    sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE),        // NodeOffset
    0
  };

  // Initialize SMMU3 Structure
  SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE  Smmu3 = {
    {
      {
        EFI_ACPI_IORT_TYPE_SMMUv3,
        sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE),
        5,                                                               // Revision
        0,                                                               // Identifier
        1,                                                               // NumIdMapping
        OFFSET_OF (SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE, SmmuIdMap) // IdReference
      },
      PcdGet64 (PcdSmmuBase),                   // Base address
      EFI_ACPI_IORT_SMMUv3_FLAG_COHAC_OVERRIDE, // Flags
      0,                                        // Reserved
      0,                                        // VATOS address
      EFI_ACPI_IORT_SMMUv3_MODEL_GENERIC,       // SMMUv3 Model
      74,                                       // Event
      75,                                       // Pri
      77,                                       // Gerror
      76,                                       // Sync
      0,                                        // Proximity domain
      1                                         // DevIDMappingIndex
    },
    {
      0x0000,                                           // InputBase
      0xffff,                                           // NumIds
      0x0000,                                           // OutputBase
      OFFSET_OF (SBSA_IO_REMAPPING_STRUCTURE, ItsNode), // OutputReference
      0                                                 // Flags
    }
  };

  SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE  Rc = {
    {
      {
        EFI_ACPI_IORT_TYPE_ROOT_COMPLEX,                            // Type
        sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE),            // Length
        0,                                                          // Revision
        0,                                                          // Identifier
        1,                                                          // NumIdMappings
        OFFSET_OF (SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE, RcIdMap) // IdReference
      },
      1,                                          // CacheCoherentAttribute
      0,                                          // AllocationHints
      0,                                          // Reserved
      1,                                          // MemoryAccessFlags
      EFI_ACPI_IORT_ROOT_COMPLEX_ATS_UNSUPPORTED, // AtsAttribute
      0x0,                                        // PciSegmentNumber
      0,                                          // MemoryAddressSize
      0,                                          // PasidCapabilities
      { 0 },                                      // Reserved1[1]
      0,                                          // Flags
    },
    {
      0x0000,                                            // InputBase
      0xffff,                                            // NumIds
      0x0000,                                            // OutputBase
      OFFSET_OF (SBSA_IO_REMAPPING_STRUCTURE, SmmuNode), // OutputReference
      0,                                                 // Flags
    }
  };

  SBSA_EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE  Its = {
    // EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE
    {
      // EFI_ACPI_6_0_IO_REMAPPING_NODE
      {
        EFI_ACPI_IORT_TYPE_ITS_GROUP,                     // Type
        sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE), // Length
        0,                                                // Revision
        0,                                                // Identifier
        0,                                                // NumIdMappings
        0,                                                // IdReference
      },
      1,    // ITS count
    },
    0,      // GIC ITS Identifiers
  };

  // Calculate the new table size based on the number of cores
  TableSize = sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE) +
              sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE) +
              sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE) +
              sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE);

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages for IORT table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  New = (UINT8 *)(UINTN)PageAddress;
  ZeroMem (New, TableSize);

  // Add the  ACPI Description table header
  CopyMem (New, &Header, sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE));
  ((EFI_ACPI_DESCRIPTION_HEADER *)New)->Length = TableSize;
  New                                         += sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE);

  // ITS Node
  CopyMem (New, &Its, sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE));
  New += sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE);

  // SMMUv3 Node
  CopyMem (New, &Smmu3, sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE));
  New += sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE);

  // RC Node
  CopyMem (New, &Rc, sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE));
  New += sizeof (SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE);

  AcpiPlatformChecksum ((UINT8 *)PageAddress, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)PageAddress,
                        TableSize,
                        &TableHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install IORT table\n"));
  }

  return Status;
}

/*
 * A function that add the MADT ACPI table.
  IN EFI_ACPI_COMMON_HEADER    *CurrentTable
 */
EFI_STATUS
AddMadtTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS            Status;
  UINTN                 TableHandle;
  UINT32                TableSize;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINT8                 *New;
  UINT32                NumCores;
  UINT32                CoreIndex;

  // Initialize MADT ACPI Header
  EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  Header = {
    SBSAQEMU_ACPI_HEADER (
      EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER,
      EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION
      ),
    0, 0
  };

  // Initialize GICC Structure
  EFI_ACPI_6_0_GIC_STRUCTURE  Gicc = EFI_ACPI_6_0_GICC_STRUCTURE_INIT (
                                       0,                         /* GicID */
                                       0,                         /* AcpiCpuUid */
                                       0,                         /* Mpidr */
                                       EFI_ACPI_6_0_GIC_ENABLED,  /* Flags */
                                       SBSAQEMU_MADT_GIC_PMU_IRQ, /* PMU Irq */
                                       0,                         /* PhysicalBaseAddress */
                                       SBSAQEMU_MADT_GIC_VBASE,   /* GicVBase */
                                       SBSAQEMU_MADT_GIC_HBASE,   /* GicHBase */
                                       25,                        /* GsivId */
                                       0,                         /* GicRBase */
                                       0                          /* Efficiency */
                                       );

  // Initialize GIC Distributor Structure
  EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE  Gicd =
    EFI_ACPI_6_0_GIC_DISTRIBUTOR_INIT (
      0,
      PcdGet64 (PcdGicDistributorBase),
      0,
      3 /* GicVersion */
      );

  // Initialize GIC Redistributor Structure
  EFI_ACPI_6_0_GICR_STRUCTURE  Gicr = SBSAQEMU_MADT_GICR_INIT ();

  NumCores = GetCpuCount ();

  // Calculate the new table size based on the number of cores
  TableSize = sizeof (EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) +
              (sizeof (EFI_ACPI_6_0_GIC_STRUCTURE) * NumCores) +
              sizeof (EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE) +
              sizeof (EFI_ACPI_6_0_GICR_STRUCTURE);

  // Initialize GIC ITS Structure
  EFI_ACPI_6_5_GIC_ITS_STRUCTURE  Gic_Its = SBSAQEMU_MADT_GIC_ITS_INIT (0);

  if (GicItsBase > 0) {
    TableSize += sizeof (EFI_ACPI_6_5_GIC_ITS_STRUCTURE);
  }

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages for MADT table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  New = (UINT8 *)(UINTN)PageAddress;
  ZeroMem (New, TableSize);

  // Add the  ACPI Description table header
  CopyMem (New, &Header, sizeof (EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER));
  ((EFI_ACPI_DESCRIPTION_HEADER *)New)->Length = TableSize;
  New                                         += sizeof (EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  // Add new GICC structures for the Cores
  for (CoreIndex = 0; CoreIndex < NumCores; CoreIndex++) {
    EFI_ACPI_6_0_GIC_STRUCTURE  *GiccPtr;

    CopyMem (New, &Gicc, sizeof (EFI_ACPI_6_0_GIC_STRUCTURE));
    GiccPtr                   = (EFI_ACPI_6_0_GIC_STRUCTURE *)New;
    GiccPtr->AcpiProcessorUid = CoreIndex;
    GiccPtr->MPIDR            = GetMpidr (CoreIndex);
    New                      += sizeof (EFI_ACPI_6_0_GIC_STRUCTURE);
  }

  // GIC Distributor Structure
  CopyMem (New, &Gicd, sizeof (EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE));
  New += sizeof (EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE);

  // GIC ReDistributor Structure
  CopyMem (New, &Gicr, sizeof (EFI_ACPI_6_0_GICR_STRUCTURE));
  New += sizeof (EFI_ACPI_6_0_GICR_STRUCTURE);

  if (GicItsBase > 0) {
    // GIC ITS Structure
    CopyMem (New, &Gic_Its, sizeof (EFI_ACPI_6_5_GIC_ITS_STRUCTURE));
    New += sizeof (EFI_ACPI_6_5_GIC_ITS_STRUCTURE);
  }

  AcpiPlatformChecksum ((UINT8 *)PageAddress, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)PageAddress,
                        TableSize,
                        &TableHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install MADT table\n"));
  }

  return Status;
}

/*
 * Function to calculate the PkgLength field in ACPI tables
 */
STATIC
UINT32
SetPkgLength (
  IN UINT8   *TablePtr,
  IN UINT32  Length
  )
{
  UINT8  ByteCount;
  UINT8  *PkgLeadByte = TablePtr;

  // Increase Payload Length to include the size of the Length Field
  if (Length <= (0x3F - 1)) {
    Length += 1;
  } else if (Length <= (0xFFF - 2)) {
    Length += 2;
  } else if (Length <= (0xFFFFF - 3)) {
    Length += 3;
  } else if (Length <= (0xFFFFFFF - 4)) {
    Length += 4;
  } else {
    DEBUG ((DEBUG_ERROR, "Failed to set PkgLength: too large\n"));
  }

  // Smaller payloads fit into a single length byte
  if (Length < 64) {
    *TablePtr = Length;
    return 1;
  }

  // Set the LSB of Length in PkgLeadByte and advance Length
  *PkgLeadByte = Length & 0xF;
  Length       = Length >> 4;

  while (Length) {
    TablePtr++;
    *TablePtr = (Length & 0xFF);
    Length    = (Length >> 8);
  }

  // Calculate the number of bytes the Length field uses
  // and set the ByteCount field in PkgLeadByte.
  ByteCount     = (TablePtr - PkgLeadByte) & 0xF;
  *PkgLeadByte |= (ByteCount << 6);

  return ByteCount + 1;
}

/*
 * A function that adds SSDT ACPI table.
 */
EFI_STATUS
AddSsdtTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS            Status;
  UINTN                 TableHandle;
  UINT32                TableSize;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINT8                 *New;
  UINT8                 *HeaderAddr;
  UINT32                CpuId;
  UINT32                Offset;
  UINT8                 ScopeOpName[] =  SBSAQEMU_ACPI_SCOPE_NAME;
  UINT32                NumCores      = GetCpuCount ();

  EFI_ACPI_DESCRIPTION_HEADER  Header =
    SBSAQEMU_ACPI_HEADER (
      EFI_ACPI_6_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_DESCRIPTION_HEADER,
      EFI_ACPI_6_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_REVISION
      );

  SBSAQEMU_ACPI_CPU_DEVICE  CpuDevice = {
    { AML_EXT_OP, AML_EXT_DEVICE_OP }, /* Device () */
    SBSAQEMU_ACPI_CPU_DEV_LEN,         /* Length */
    SBSAQEMU_ACPI_CPU_DEV_NAME,        /* Device Name "C000" */
    SBSAQEMU_ACPI_CPU_HID,             /* Name (HID, "ACPI0007") */
    SBSAQEMU_ACPI_CPU_UID,             /* Name (UID, 0) */
  };

  // Calculate the new table size based on the number of cores
  TableSize = sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
              SBSAQEMU_ACPI_SCOPE_OP_MAX_LENGTH + sizeof (ScopeOpName) +
              (sizeof (CpuDevice) * NumCores);

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages for SSDT table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  HeaderAddr = New = (UINT8 *)(UINTN)PageAddress;
  ZeroMem (New, TableSize);

  // Add the ACPI Description table header
  CopyMem (New, &Header, sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  New += sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  // Insert the top level ScopeOp
  *New = AML_SCOPE_OP;
  New++;
  Offset = SetPkgLength (
             New,
             (sizeof (ScopeOpName) + (sizeof (CpuDevice) * NumCores))
             );

  // Adjust TableSize now we know header length of _SB
  TableSize                                          -= (SBSAQEMU_ACPI_SCOPE_OP_MAX_LENGTH - (Offset + 1));
  ((EFI_ACPI_DESCRIPTION_HEADER *)HeaderAddr)->Length = TableSize;

  New += Offset;
  CopyMem (New, &ScopeOpName, sizeof (ScopeOpName));
  New += sizeof (ScopeOpName);

  // Add new Device structures for the Cores
  for (CpuId = 0; CpuId < NumCores; CpuId++) {
    SBSAQEMU_ACPI_CPU_DEVICE  *CpuDevicePtr;

    CopyMem (New, &CpuDevice, sizeof (SBSAQEMU_ACPI_CPU_DEVICE));
    CpuDevicePtr = (SBSAQEMU_ACPI_CPU_DEVICE *)New;

    AsciiSPrint ((CHAR8 *)&CpuDevicePtr->dev_name[1], 4, "%03X", CpuId);

    /* replace character lost by above NULL termination */
    CpuDevicePtr->hid[0] = AML_NAME_OP;

    CpuDevicePtr->uid[6] = CpuId & 0xFF;
    CpuDevicePtr->uid[7] = (CpuId >> 8) & 0xFF;
    New                 += sizeof (SBSAQEMU_ACPI_CPU_DEVICE);
  }

  // Perform Checksum
  AcpiPlatformChecksum ((UINT8 *)PageAddress, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)PageAddress,
                        TableSize,
                        &TableHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install SSDT table\n"));
  }

  return Status;
}

STATIC VOID
AcpiPpttFillCacheSizeInfo (
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE  *Node,
  UINT32                             Level,
  BOOLEAN                            DataCache,
  BOOLEAN                            UnifiedCache
  )
{
  CSSELR_DATA  CsselrData;
  CCSIDR_DATA  CcsidrData;

  CsselrData.Data       = 0;
  CsselrData.Bits.Level = Level - 1;
  CsselrData.Bits.InD   = (!DataCache && !UnifiedCache);

  CcsidrData.Data = ReadCCSIDR (CsselrData.Data);

  Node->Flags.LineSizeValid      = 1;
  Node->Flags.NumberOfSetsValid  = 1;
  Node->Flags.AssociativityValid = 1;
  Node->Flags.SizePropertyValid  = 1;
  Node->Flags.CacheTypeValid     = 1;

  if (ArmHasCcidx ()) {
    Node->NumberOfSets  = CcsidrData.BitsCcidxAA64.NumSets + 1;
    Node->Associativity = CcsidrData.BitsCcidxAA64.Associativity + 1;
    Node->LineSize      = (1 << (CcsidrData.BitsCcidxAA64.LineSize + 4));
  } else {
    Node->NumberOfSets  = (UINT16)CcsidrData.BitsNonCcidx.NumSets + 1;
    Node->Associativity = (UINT16)CcsidrData.BitsNonCcidx.Associativity + 1;
    Node->LineSize      = (UINT16)(1 << (CcsidrData.BitsNonCcidx.LineSize + 4));
  }

  Node->Size = Node->NumberOfSets *
               Node->Associativity *
               Node->LineSize;
}

STATIC
UINT32
AddCoresToPpttTable (
  UINT8        *New,
  UINT32       ClusterOffset,
  CpuTopology  CpuTopo
  )
{
  UINT32  L1DCacheOffset;
  UINT32  L1ICacheOffset;
  UINT32  L2CacheOffset;
  UINT32  CoreOffset;
  UINT32  Offset;
  UINT32  CoreCpuId;
  UINT32  CoreIndex;
  UINT32  ThreadIndex;
  UINT32  *PrivateResourcePtr;

  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_FLAGS  CoreFlags = {
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_VALID,
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,
    EFI_ACPI_6_5_PPTT_NODE_IS_LEAF,
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL
  };

  if (CpuTopo.Threads > 1) {
    // The Thread structure is the leaf structure, adjust the value of CoreFlags.
    CoreFlags.AcpiProcessorIdValid = EFI_ACPI_6_5_PPTT_PROCESSOR_ID_INVALID;
    CoreFlags.NodeIsALeaf          = EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF;
  }

  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_FLAGS  ThreadFlags = {
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_VALID,
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_THREAD,
    EFI_ACPI_6_5_PPTT_NODE_IS_LEAF,
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL
  };

  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE  L1DCache = SBSAQEMU_ACPI_PPTT_L1_D_CACHE_STRUCT;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE  L1ICache = SBSAQEMU_ACPI_PPTT_L1_I_CACHE_STRUCT;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE  L2Cache  = SBSAQEMU_ACPI_PPTT_L2_CACHE_STRUCT;

  AcpiPpttFillCacheSizeInfo (&L1DCache, 1, TRUE, FALSE);
  AcpiPpttFillCacheSizeInfo (&L1ICache, 1, FALSE, FALSE);
  AcpiPpttFillCacheSizeInfo (&L2Cache, 2, FALSE, TRUE);

  CoreOffset = ClusterOffset + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
  Offset     = CoreOffset;

  for (CoreIndex = 0; CoreIndex < CpuTopo.Cores; CoreIndex++) {
    if (CpuTopo.Threads == 1) {
      CoreCpuId = mCpuId;
    } else {
      CoreCpuId = 0;
    }

    // space for Core + PrivateResourcePtr
    Offset += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
    Offset += sizeof (UINT32) * 2;

    L1DCacheOffset = Offset;
    L1ICacheOffset = L1DCacheOffset + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);
    L2CacheOffset  = L1ICacheOffset + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

    EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR  Core = SBSAQEMU_ACPI_PROCESSOR_HIERARCHY_NODE_STRUCTURE_INIT (
                                                    CoreFlags,
                                                    ClusterOffset,
                                                    CoreCpuId,
                                                    2
                                                    );

    CopyMem (New, &Core, sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR));
    New += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);

    PrivateResourcePtr    = (UINT32 *)New;
    PrivateResourcePtr[0] = L1DCacheOffset;
    PrivateResourcePtr[1] = L1ICacheOffset;
    New                  += (2 * sizeof (UINT32));

    // Add L1 D Cache structure
    L1DCache.CacheId = mCacheId++;
    CopyMem (New, &L1DCache, sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE));
    ((EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE *)New)->NextLevelOfCache = L2CacheOffset;
    New                                                         += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

    // Add L1 I Cache structure
    L1ICache.CacheId = mCacheId++;
    CopyMem (New, &L1ICache, sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE));
    ((EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE *)New)->NextLevelOfCache = L2CacheOffset;
    New                                                         += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

    // Add L2 Cache structure
    L2Cache.CacheId = mCacheId++;
    CopyMem (New, &L2Cache, sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE));
    New += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

    Offset += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE) * 3;

    if (CpuTopo.Threads == 1) {
      mCpuId++;
    } else {
      // Add the Thread PPTT structure
      for (ThreadIndex = 0; ThreadIndex < CpuTopo.Threads; ThreadIndex++) {
        EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR  Thread = SBSAQEMU_ACPI_PROCESSOR_HIERARCHY_NODE_STRUCTURE_INIT (
                                                          ThreadFlags,
                                                          CoreOffset,
                                                          mCpuId,
                                                          0
                                                          );
        CopyMem (New, &Thread, sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR));
        New += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
        mCpuId++;
      }

      Offset +=  CpuTopo.Threads * sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
    }

    CoreOffset = Offset;
  }

  return CoreOffset - ClusterOffset;
}

/*
 * A function that adds the PPTT ACPI table.
 */
EFI_STATUS
AddPpttTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS            Status;
  UINTN                 TableHandle;
  UINT32                TableSize;
  UINT32                CoresPartSize;
  UINT32                SocketIndex;
  UINT32                ClusterIndex;
  UINT32                SocketOffset;
  UINT32                ClusterOffset;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINT8                 *New;
  CpuTopology           CpuTopo;

  GetCpuTopology (&CpuTopo);

  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_FLAGS  SocketFlags = {
    EFI_ACPI_6_5_PPTT_PACKAGE_PHYSICAL,
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_INVALID,
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,
    EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF,
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL
  };

  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_FLAGS  ClusterFlags = {
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_INVALID,
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,
    EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF,
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL
  };

  EFI_ACPI_DESCRIPTION_HEADER  Header =
    SBSAQEMU_ACPI_HEADER (
      EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
      EFI_ACPI_DESCRIPTION_HEADER,
      EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION
      );

  TableSize = sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
              CpuTopo.Sockets * (sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR) +
                                 CpuTopo.Clusters * (sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR) +
                                                     CpuTopo.Cores * (sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR) +
                                                                      sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE) * 3 +
                                                                      sizeof (UINT32) * 2)));

  if (CpuTopo.Threads > 1) {
    TableSize += CpuTopo.Sockets * CpuTopo.Clusters * CpuTopo.Cores * CpuTopo.Threads *
                 sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
  }

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages for PPTT table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  New = (UINT8 *)(UINTN)PageAddress;
  ZeroMem (New, TableSize);

  // Add the ACPI Description table header
  CopyMem (New, &Header, sizeof (EFI_ACPI_DESCRIPTION_HEADER));
  ((EFI_ACPI_DESCRIPTION_HEADER *)New)->Length = TableSize;
  New                                         += sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  mCpuId   = 0;
  mCacheId = 1;     // 0 is not a valid Cache ID.

  SocketOffset = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
  for (SocketIndex = 0; SocketIndex < CpuTopo.Sockets; SocketIndex++) {
    // Add the Socket PPTT structure
    EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR  Socket = SBSAQEMU_ACPI_PROCESSOR_HIERARCHY_NODE_STRUCTURE_INIT (
                                                      SocketFlags,
                                                      0,
                                                      0,
                                                      0
                                                      );
    CopyMem (New, &Socket, sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR));
    New += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);

    ClusterOffset = SocketOffset + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
    for (ClusterIndex = 0; ClusterIndex < CpuTopo.Clusters; ClusterIndex++) {
      // Add the Cluster PPTT structure
      EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR  Cluster = SBSAQEMU_ACPI_PROCESSOR_HIERARCHY_NODE_STRUCTURE_INIT (
                                                         ClusterFlags,
                                                         SocketOffset,
                                                         0,
                                                         0
                                                         );
      CopyMem (New, &Cluster, sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR));
      New += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);

      CoresPartSize  = AddCoresToPpttTable (New, ClusterOffset, CpuTopo);
      ClusterOffset += CoresPartSize;
      New           += CoresPartSize - sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
    }

    SocketOffset = ClusterOffset;
  }

  // Perform Checksum
  AcpiPlatformChecksum ((UINT8 *)PageAddress, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)PageAddress,
                        TableSize,
                        &TableHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install PPTT table\n"));
  }

  return Status;
}

/*
 * A function that adds the GTDT ACPI table.
 */
EFI_STATUS
AddGtdtTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS            Status;
  UINTN                 TableHandle;
  UINT32                TableSize;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINT8                 *New;

  TableSize = sizeof (EFI_ACPI_6_5_GENERIC_TIMER_DESCRIPTION_TABLE) +
              sizeof (EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE);

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages for GTDT table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE  Gtdt = {
    SBSAQEMU_ACPI_HEADER (
      EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
      GENERIC_TIMER_DESCRIPTION_TABLES,
      EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION
      ),

    SYSTEM_TIMER_BASE_ADDRESS,                      // UINT64  PhysicalAddress
    0,                                              // UINT32  Reserved
    FixedPcdGet32 (PcdArmArchTimerSecIntrNum),      // UINT32  SecurePL1TimerGSIV
    GTDT_GTIMER_FLAGS,                              // UINT32  SecurePL1TimerFlags
    FixedPcdGet32 (PcdArmArchTimerIntrNum),         // UINT32  NonSecurePL1TimerGSIV
    GTDT_GTIMER_FLAGS,                              // UINT32  NonSecurePL1TimerFlags
    FixedPcdGet32 (PcdArmArchTimerVirtIntrNum),     // UINT32  VirtualTimerGSIV
    GTDT_GTIMER_FLAGS,                              // UINT32  VirtualTimerFlags
    FixedPcdGet32 (PcdArmArchTimerHypIntrNum),      // UINT32  NonSecurePL2TimerGSIV
    GTDT_GTIMER_FLAGS,                              // UINT32  NonSecurePL2TimerFlags
    MAX_ADDRESS,                                    // UINT64  CntReadBasePhysicalAddress
    SBSA_PLATFORM_TIMER_COUNT,                      // UINT32  PlatformTimerCount
    sizeof (EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE),
    // UINT32  PlatformTimerOffset
    FixedPcdGet32 (PcdArmArchTimerHypVirtIntrNum),  // UINT32  VirtualPL2TimerGSIV
    GTDT_GTIMER_FLAGS                               // UINT32  VirtualPL2TimerFlags
  };

  // Non-secure EL2 virtual timer requires VHE support (v8.1+)
  if (!ArmHasVhe ()) {
    Gtdt.VirtualPL2TimerGSIV  = 0;
    Gtdt.VirtualPL2TimerFlags = 0;
  }

  EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE  Gwdt = {
    EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG,
    sizeof (EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE),
    EFI_ACPI_RESERVED_WORD,
    SBSAQEMU_WDT_REFRESH_FRAME_BASE,
    SBSAQEMU_WDT_CONTROL_FRAME_BASE,
    SBSAQEMU_WDT_IRQ,
    GTDT_WDTIMER_FLAGS
  };

  New = (UINT8 *)(UINTN)PageAddress;
  ZeroMem (New, TableSize);

  CopyMem (New, &Gtdt, sizeof (EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE));
  New += sizeof (EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE);

  CopyMem (New, &Gwdt, sizeof (EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE));
  New += sizeof (EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE);

  // Perform Checksum
  AcpiPlatformChecksum ((UINT8 *)PageAddress, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)PageAddress,
                        TableSize,
                        &TableHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install GTDT table\n"));
  }

  return Status;
}

/*
 * A function that adds the SRAT ACPI table.
 */
EFI_STATUS
AddSratTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS            Status;
  UINT8                 *New;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINTN                 TableHandle;
  UINT32                TableSize;
  UINT32                Index;
  UINT32                NodeId;
  UINT32                NumMemNodes;
  MemoryInfo            MemInfo;
  UINT32                NumCores = GetCpuCount ();

  // Initialize SRAT ACPI Header
  EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER  Header = {
    SBSAQEMU_ACPI_HEADER (
      EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE,
      EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER,
      EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_REVISION
      ),
    1, 0
  };

  NumMemNodes = GetMemNodeCount ();

  // Calculate the new table size based on the number of cores
  TableSize = sizeof (EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER) +
              (sizeof (EFI_ACPI_6_4_MEMORY_AFFINITY_STRUCTURE) * NumMemNodes) +
              (sizeof (EFI_ACPI_6_4_GICC_AFFINITY_STRUCTURE) * NumCores);

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages for SRAT table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  New = (UINT8 *)(UINTN)PageAddress;
  ZeroMem (New, TableSize);

  // Add the ACPI Description table header
  CopyMem (New, &Header, sizeof (EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER));
  ((EFI_ACPI_DESCRIPTION_HEADER *)New)->Length = TableSize;
  New                                         += sizeof (EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER);

  // Add memory structures
  for (Index = 0; Index < NumMemNodes; Index++) {
    GetMemInfo (Index, &MemInfo);
    EFI_ACPI_6_4_MEMORY_AFFINITY_STRUCTURE  memory = SBSAQEMU_ACPI_MEMORY_AFFINITY_STRUCTURE_INIT (MemInfo.NodeId, MemInfo.AddressBase, MemInfo.AddressSize, 1);
    CopyMem (New, &memory, sizeof (EFI_ACPI_6_4_MEMORY_AFFINITY_STRUCTURE));
    New += sizeof (EFI_ACPI_6_4_MEMORY_AFFINITY_STRUCTURE);
  }

  // Add processor structures for the cores
  for (Index = 0; Index < NumCores; Index++) {
    NodeId = GetCpuNumaNode (Index);
    EFI_ACPI_6_4_GICC_AFFINITY_STRUCTURE  gicc = SBSAQEMU_ACPI_GICC_AFFINITY_STRUCTURE_INIT (NodeId, Index, 1, 0);
    CopyMem (New, &gicc, sizeof (EFI_ACPI_6_4_GICC_AFFINITY_STRUCTURE));
    New += sizeof (EFI_ACPI_6_4_GICC_AFFINITY_STRUCTURE);
  }

  // Perform Checksum
  AcpiPlatformChecksum ((UINT8 *)PageAddress, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)PageAddress,
                        TableSize,
                        &TableHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install SRAT table\n"));
  }

  return Status;
}

/*
 * A function to disable XHCI node on Platform Version lower than 0.3
 */
STATIC
EFI_STATUS
DisableXhciOnOlderPlatVer (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_SDT_PROTOCOL        *AcpiSdtProtocol;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;
  UINTN                        TableKey;
  UINTN                        TableIndex;
  EFI_ACPI_HANDLE              TableHandle;

  Status = EFI_SUCCESS;

  if ( PLATFORM_VERSION_LESS_THAN (0, 3)) {
    DEBUG ((DEBUG_ERROR, "Platform Version < 0.3 - disabling XHCI\n"));
    Status = gBS->LocateProtocol (
                    &gEfiAcpiSdtProtocolGuid,
                    NULL,
                    (VOID **)&AcpiSdtProtocol
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Unable to locate ACPI table protocol\n"));
      return Status;
    }

    Status = AcpiLocateTableBySignature (
               AcpiSdtProtocol,
               EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
               &TableIndex,
               &Table,
               &TableKey
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ACPI DSDT table not found!\n"));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = AcpiSdtProtocol->OpenSdt (TableKey, &TableHandle);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      AcpiSdtProtocol->Close (TableHandle);
      return Status;
    }

    Status = AcpiAmlObjectUpdateInteger (AcpiSdtProtocol, TableHandle, "\\_SB.USB0.XHCI", 0x0);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to disable XHCI!\n"));
      ASSERT_EFI_ERROR (Status);
      AcpiSdtProtocol->Close (TableHandle);
      return Status;
    }

    AcpiSdtProtocol->Close (TableHandle);
    AcpiUpdateChecksum ((UINT8 *)Table, Table->Length);
  }

  return Status;
}

EFI_STATUS
EFIAPI
InitializeSbsaQemuAcpiDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;

  // Check if ACPI Table Protocol has been installed
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI Table Protocol\n"));
    return Status;
  }

  GicItsBase = PcdGet64 (PcdGicItsBase);

  if (GicItsBase > 0) {
    Status = AddIortTable (AcpiTable);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to add IORT table\n"));
    }
  }

  Status = AddMadtTable (AcpiTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to add MADT table\n"));
  }

  Status = AddSsdtTable (AcpiTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to add SSDT table\n"));
  }

  Status = AddPpttTable (AcpiTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to add PPTT table\n"));
  }

  if (GetNumaNodeCount () > 1) {
    Status = AddSratTable (AcpiTable);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to add SRAT table\n"));
    }
  }

  Status = AddGtdtTable (AcpiTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to add GTDT table\n"));
  }

  Status = DisableXhciOnOlderPlatVer ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to handle XHCI enablement\n"));
  }

  return EFI_SUCCESS;
}
