/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/ArmCache.h>
#include <Library/AmpereCpuLib.h>
#include <Library/ArmLib.h>
#include <Guid/CpuConfigHii.h>
#include <CpuConfigNVDataStruc.h>
#include "AcpiPlatform.h"

#define NUMBER_OF_RESOURCES  2 // L1I & L1D

EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR  PpttProcessorTemplate = {
  EFI_ACPI_6_3_PPTT_TYPE_PROCESSOR,
  sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR),
  { EFI_ACPI_RESERVED_BYTE,                      EFI_ACPI_RESERVED_BYTE},
  { 0 },      // Flags
  0,          // Parent
  0,          // AcpiProcessorId
  0           // NumberOfPrivateResources
};

EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE  PpttCacheTemplate = {
  EFI_ACPI_6_3_PPTT_TYPE_CACHE,
  sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE),
  { EFI_ACPI_RESERVED_BYTE,                  EFI_ACPI_RESERVED_BYTE},
  { 0 },                                     // Flags
  0,                                         // NextLevelOfCache
  0,                                         // Size
  0,                                         // NumberOfSets
  0,                                         // Associativity
  { 0 },                                     // Attributes
  0
};

EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER  PpttTableHeaderTemplate = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
    0,
    EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION
    )
};

UINT32
AddProcessorCoreNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR  *PpttProcessorEntryPointer,
  UINT32                                 CpuId,
  UINT32                                 ParentOffset,
  UINT32                                 L1INodeOffset,
  UINT32                                 L1DNodeOffset
  )
{
  UINT32  *ResPointer;
  UINTN   ClusterIdPerSocket;
  UINTN   CoreIdPerCpm;
  UINTN   SocketId;

  ASSERT (CpuId < PLATFORM_CPU_MAX_NUM_CORES);

  ClusterIdPerSocket = CLUSTER_ID (CpuId);
  SocketId           = SOCKET_ID (CpuId);
  CoreIdPerCpm       = CpuId % PLATFORM_CPU_NUM_CORES_PER_CPM;

  CopyMem (
    PpttProcessorEntryPointer,
    &PpttProcessorTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR)
    );

  PpttProcessorEntryPointer->Flags.AcpiProcessorIdValid    = EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID;
  PpttProcessorEntryPointer->Flags.NodeIsALeaf             = EFI_ACPI_6_3_PPTT_NODE_IS_LEAF;
  PpttProcessorEntryPointer->Flags.IdenticalImplementation = EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL;
  PpttProcessorEntryPointer->AcpiProcessorId               = (SocketId << PLATFORM_SOCKET_UID_BIT_OFFSET) | (ClusterIdPerSocket << 8) | CoreIdPerCpm;
  PpttProcessorEntryPointer->Parent                        = ParentOffset;
  PpttProcessorEntryPointer->NumberOfPrivateResources      = NUMBER_OF_RESOURCES;
  PpttProcessorEntryPointer->Length                       += sizeof (UINT32) * PpttProcessorEntryPointer->NumberOfPrivateResources;

  ResPointer = (UINT32 *)((UINT64)PpttProcessorEntryPointer + sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR));

  ResPointer[0] = L1INodeOffset;
  ResPointer[1] = L1DNodeOffset;

  return PpttProcessorEntryPointer->Length;
}

UINT32
AddClusterNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR  *PpttProcessorEntryPointer,
  UINT32                                 ParentOffset
  )
{
  CopyMem (
    PpttProcessorEntryPointer,
    &PpttProcessorTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR)
    );

  PpttProcessorEntryPointer->Parent                        = ParentOffset;
  PpttProcessorEntryPointer->Flags.IdenticalImplementation = EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL;

  return PpttProcessorEntryPointer->Length;
}

UINT32
AddSocketNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR  *PpttProcessorEntryPointer,
  UINT32                                 ParentOffset,
  UINT32                                 SlcNodeOffset
  )
{
  UINT32  *ResPointer;

  CopyMem (
    PpttProcessorEntryPointer,
    &PpttProcessorTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR)
    );

  PpttProcessorEntryPointer->Flags.PhysicalPackage         = EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL;
  PpttProcessorEntryPointer->Flags.IdenticalImplementation = EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL;
  PpttProcessorEntryPointer->Parent                        = ParentOffset;
  if (SlcNodeOffset > 0) {
    PpttProcessorEntryPointer->NumberOfPrivateResources = 1;
    PpttProcessorEntryPointer->Length                  += sizeof (UINT32) * PpttProcessorEntryPointer->NumberOfPrivateResources;

    ResPointer    = (UINT32 *)((UINT64)PpttProcessorEntryPointer + sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR));
    ResPointer[0] = SlcNodeOffset;
  } else {
    PpttProcessorEntryPointer->NumberOfPrivateResources = 0;
  }

  return PpttProcessorEntryPointer->Length;
}

UINT32
AddRootNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR  *PpttProcessorEntryPointer
  )
{
  CopyMem (
    PpttProcessorEntryPointer,
    &PpttProcessorTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR)
    );

  PpttProcessorEntryPointer->Flags.IdenticalImplementation = EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL;

  return PpttProcessorEntryPointer->Length;
}

VOID
AddFillCacheSizeInfo (
  EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE  *Node,
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

  Node->Flags.SizePropertyValid  = 1;
  Node->Flags.NumberOfSetsValid  = EFI_ACPI_6_3_PPTT_NUMBER_OF_SETS_VALID;
  Node->Flags.AssociativityValid = EFI_ACPI_6_3_PPTT_ASSOCIATIVITY_VALID;
  Node->Flags.CacheTypeValid     = EFI_ACPI_6_3_PPTT_CACHE_TYPE_VALID;
  Node->Flags.LineSizeValid      = EFI_ACPI_6_3_PPTT_LINE_SIZE_VALID;
  Node->NumberOfSets             = (UINT16)CcsidrData.BitsNonCcidx.NumSets + 1;
  Node->Associativity            = (UINT16)CcsidrData.BitsNonCcidx.Associativity + 1;
  Node->LineSize                 = (UINT16)(1 << (CcsidrData.BitsNonCcidx.LineSize + 4));
  Node->Size                     = Node->NumberOfSets *
                                   Node->Associativity *
                                   Node->LineSize;
}

UINT32
AddL1DataCacheNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE  *PpttCacheEntryPointer,
  UINT32                             NextCacheLevelOffset
  )
{
  CopyMem (
    PpttCacheEntryPointer,
    &PpttCacheTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE)
    );

  AddFillCacheSizeInfo (PpttCacheEntryPointer, 1, TRUE, FALSE);
  PpttCacheEntryPointer->Attributes.CacheType = EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA;
  PpttCacheEntryPointer->NextLevelOfCache     = NextCacheLevelOffset;

  return PpttCacheEntryPointer->Length;
}

UINT32
AddL1InstructionCacheNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE  *PpttCacheEntryPointer,
  UINT32                             NextCacheLevelOffset
  )
{
  CopyMem (
    PpttCacheEntryPointer,
    &PpttCacheTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE)
    );

  AddFillCacheSizeInfo (PpttCacheEntryPointer, 1, FALSE, FALSE);
  PpttCacheEntryPointer->Attributes.CacheType = EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION;
  PpttCacheEntryPointer->NextLevelOfCache     = NextCacheLevelOffset;

  return PpttCacheEntryPointer->Length;
}

UINT32
AddL2CacheNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE  *PpttCacheEntryPointer
  )
{
  CopyMem (
    PpttCacheEntryPointer,
    &PpttCacheTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE)
    );

  AddFillCacheSizeInfo (PpttCacheEntryPointer, 2, FALSE, TRUE);
  PpttCacheEntryPointer->Attributes.CacheType = EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED;
  PpttCacheEntryPointer->NextLevelOfCache     = 0;

  return PpttCacheEntryPointer->Length;
}

UINT32
AddSlcCacheNode (
  EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE  *PpttCacheEntryPointer
  )
{
  CopyMem (
    PpttCacheEntryPointer,
    &PpttCacheTemplate,
    sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE)
    );

  PpttCacheEntryPointer->Flags.SizePropertyValid  = 1;
  PpttCacheEntryPointer->Flags.LineSizeValid      = EFI_ACPI_6_3_PPTT_LINE_SIZE_VALID;
  PpttCacheEntryPointer->Flags.NumberOfSetsValid  = EFI_ACPI_6_3_PPTT_NUMBER_OF_SETS_VALID;
  PpttCacheEntryPointer->Flags.AssociativityValid = EFI_ACPI_6_3_PPTT_ASSOCIATIVITY_VALID;
  PpttCacheEntryPointer->Flags.CacheTypeValid     = EFI_ACPI_6_3_PPTT_CACHE_TYPE_VALID;

  if (IsAc01Processor ()) {
    PpttCacheEntryPointer->Size = SIZE_32MB;
  } else {
    PpttCacheEntryPointer->Size = SIZE_16MB;
  }

  PpttCacheEntryPointer->NumberOfSets = 0x400;     /* 1024 sets per 1MB HN-F */

  PpttCacheEntryPointer->Associativity    = 0x10; /* 16-way set-associative */
  PpttCacheEntryPointer->LineSize         = 0x40; /* 64 bytes */
  PpttCacheEntryPointer->NextLevelOfCache = 0;

  PpttCacheEntryPointer->Attributes.CacheType = 0x3; /* Unified Cache */

  return PpttCacheEntryPointer->Length;
}

/*
 *  Install PPTT table.
 */
EFI_STATUS
AcpiInstallPpttTable (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  EFI_STATUS               Status;
  UINT32                   ClusterNodeOffset[PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_MAX_SOCKET];
  UINT32                   L1DNodeOffset;
  UINT32                   L1INodeOffset;
  UINT32                   L2NodeOffset;
  UINT32                   SlcNodeOffset;
  UINT32                   RootNodeOffset;
  UINT32                   SocketNodeOffset[PLATFORM_CPU_MAX_SOCKET];
  UINTN                    ClusterId;
  UINTN                    CpuId;
  UINTN                    NextNodeOffset;
  UINTN                    NumSockets;
  UINTN                    PpttTableKey;
  UINTN                    Size;
  UINTN                    SocketId;
  VOID                     *PpttTablePointer;
  CPU_VARSTORE_DATA        CpuConfigData;
  UINTN                    BufferSize;

  BufferSize = sizeof (CPU_VARSTORE_DATA);
  ZeroMem (&CpuConfigData, BufferSize);
  Status = gRT->GetVariable (
                  CPU_CONFIG_VARIABLE_NAME,
                  &gCpuConfigFormSetGuid,
                  NULL,
                  &BufferSize,
                  &CpuConfigData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Could not get CPU Configuration Data \n", __func__));
  }

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NumSockets = GetNumberOfActiveSockets ();

  Size = sizeof (EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER) +
         sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR) +                                                                       // Root node
         sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR) * NumSockets +                                                          // Socket node
         sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR) * (GetNumberOfActiveCores () / PLATFORM_CPU_NUM_CORES_PER_CPM) +        // Cluster node
         (sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR) + NUMBER_OF_RESOURCES * sizeof (UINT32)) * GetNumberOfActiveCores () + // Core node
         sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE) +                                                                           // L1I node
         sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE) +                                                                           // L1D node
         sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE);                                                                            // L2 node
  if (CpuConfigData.CpuSlcAsL3 == CPU_SLC_AS_L3_ENABLE) {
    Size += sizeof (EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE) + sizeof (UINT32);                                                       // SLC node
  }

  PpttTablePointer = AllocateZeroPool (Size);
  if (PpttTablePointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NextNodeOffset = sizeof (EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER);

  RootNodeOffset  = NextNodeOffset;
  NextNodeOffset += AddRootNode (PpttTablePointer + NextNodeOffset);

  SlcNodeOffset = 0;
  if (CpuConfigData.CpuSlcAsL3 == CPU_SLC_AS_L3_ENABLE) {
    SlcNodeOffset   = NextNodeOffset;
    NextNodeOffset += AddSlcCacheNode (PpttTablePointer + NextNodeOffset);
  }

  L2NodeOffset    = NextNodeOffset;
  NextNodeOffset += AddL2CacheNode (PpttTablePointer + NextNodeOffset);

  L1INodeOffset   = NextNodeOffset;
  NextNodeOffset += AddL1InstructionCacheNode (PpttTablePointer + NextNodeOffset, L2NodeOffset);

  L1DNodeOffset   = NextNodeOffset;
  NextNodeOffset += AddL1DataCacheNode (PpttTablePointer + NextNodeOffset, L2NodeOffset);

  for (SocketId = 0; SocketId < NumSockets; SocketId++) {
    SocketNodeOffset[SocketId] = NextNodeOffset;
    NextNodeOffset            += AddSocketNode (PpttTablePointer + NextNodeOffset, RootNodeOffset, SlcNodeOffset);
  }

  for (ClusterId = 0; ClusterId < PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_MAX_SOCKET; ClusterId++) {
    if (!IsCpuEnabled (ClusterId * PLATFORM_CPU_NUM_CORES_PER_CPM)) {
      continue;
    }

    ClusterNodeOffset[ClusterId] = NextNodeOffset;
    NextNodeOffset              += AddClusterNode (
                                     PpttTablePointer + NextNodeOffset,
                                     SocketNodeOffset[ClusterId / PLATFORM_CPU_MAX_CPM]
                                     );
  }

  for (CpuId = 0; CpuId < PLATFORM_CPU_MAX_NUM_CORES; CpuId++) {
    if (!IsCpuEnabled (CpuId)) {
      continue;
    }

    NextNodeOffset += AddProcessorCoreNode (
                        PpttTablePointer + NextNodeOffset,
                        CpuId,
                        ClusterNodeOffset[CpuId / PLATFORM_CPU_NUM_CORES_PER_CPM],
                        L1INodeOffset,
                        L1DNodeOffset
                        );
  }

  ASSERT (NextNodeOffset == Size);

  CopyMem (
    PpttTablePointer,
    &PpttTableHeaderTemplate,
    sizeof (EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER)
    );

  ((EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER *)PpttTablePointer)->Header.Length = Size;

  AcpiUpdateChecksum ((UINT8 *)PpttTablePointer, Size);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)PpttTablePointer,
                                Size,
                                &PpttTableKey
                                );
  ASSERT_EFI_ERROR (Status);

  FreePool (PpttTablePointer);

  return Status;
}
