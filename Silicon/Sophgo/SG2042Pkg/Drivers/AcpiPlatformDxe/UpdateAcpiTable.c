/** @file
  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FdtClient.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Include/SG2042AcpiHeader.h>

#define CORECOUNT(X) ((X) * CORE_NUM_PER_SOCKET)

STATIC
VOID
RemoveUnusedMemoryNode (
  IN OUT EFI_ACPI_STATIC_RESOURCE_AFFINITY_TABLE  *Table,
  IN     UINTN                        MemoryNodeNum
)
{
  UINTN                   CurrPtr, NewPtr;

  if (MemoryNodeNum >= EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT) {
    return;
  }

  CurrPtr = (UINTN) &(Table->Memory[EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT]);
  NewPtr = (UINTN) &(Table->Memory[MemoryNodeNum]);

  CopyMem ((VOID *)NewPtr, (VOID *)CurrPtr, (UINTN)Table + Table->Header.Header.Length - CurrPtr);

  Table->Header.Header.Length -= CurrPtr - NewPtr;

  return;
}

STATIC
EFI_STATUS
UpdateSrat (
  IN OUT EFI_ACPI_STATIC_RESOURCE_AFFINITY_TABLE *Table
  )
{
  FDT_CLIENT_PROTOCOL              *FdtClient;
  EFI_STATUS                       Status, FindNodeStatus;
  INT32                            Node;
  CONST UINT32                     *Reg;
  UINT32                           RegSize;
  UINTN                            AddressCells, SizeCells;
  UINT64                           CurBase;
  UINT64                           CurSize;
  CONST UINT32                     *NodeId;
  UINT32                           NodeIdLen;
  UINTN                            MemoryNode = 0;

  DEBUG((DEBUG_INFO, "SRAT: Updating SRAT memory information.\n"));

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Check for memory node and add the memory spaces except the lowest one
  //
  for (FindNodeStatus = FdtClient->FindMemoryNodeReg (
                                     FdtClient,
                                     &Node,
                                     (CONST VOID **)&Reg,
                                     &AddressCells,
                                     &SizeCells,
                                     &RegSize
                                     );
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextMemoryNodeReg (
                                     FdtClient,
                                     Node,
                                     &Node,
                                     (CONST VOID **)&Reg,
                                     &AddressCells,
                                     &SizeCells,
                                     &RegSize
                                     ))
  {
    ASSERT (AddressCells <= 2);
    ASSERT (SizeCells <= 2);

    while (RegSize > 0) {
      CurBase = SwapBytes32 (*Reg++);
      if (AddressCells > 1) {
        CurBase = (CurBase << 32) | SwapBytes32 (*Reg++);
      }

      CurSize = SwapBytes32 (*Reg++);
      if (SizeCells > 1) {
        CurSize = (CurSize << 32) | SwapBytes32 (*Reg++);
      }

      RegSize -= (AddressCells + SizeCells) * sizeof (UINT32);

      Status = FdtClient->GetNodeProperty (
                            FdtClient,
                            Node,
                            "numa-node-id",
                            (CONST VOID **)&NodeId,
                            &NodeIdLen
                            );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: GetNodeProperty ('numa-node-id') failed (Status == %r)\n",
          __func__,
          Status
        ));
      }

      DEBUG((DEBUG_INFO, "NodeId = %d, Base = 0x%lx, Size = 0x%lx\n", SwapBytes32 (NodeId[0]), CurBase, CurSize));
      if (CurSize > 0) {
        Table->Memory[MemoryNode].ProximityDomain = SwapBytes32 (NodeId[0]);
        Table->Memory[MemoryNode].AddressBaseLow = CurBase;
        Table->Memory[MemoryNode].AddressBaseHigh = CurBase >> 32;
        Table->Memory[MemoryNode].LengthLow = CurSize;
        Table->Memory[MemoryNode].LengthHigh = CurSize >> 32;
        MemoryNode = MemoryNode + 1;
      }
    }
  }

  //remove invalid memory node
  RemoveUnusedMemoryNode (Table, MemoryNode);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UpdateSlit (
  IN OUT EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  return  EFI_SUCCESS;
}

EFI_STATUS
UpdateAcpiTable (
  IN OUT EFI_ACPI_DESCRIPTION_HEADER      *TableHeader
)
{
  EFI_STATUS Status = EFI_SUCCESS;

  switch (TableHeader->Signature) {

  case EFI_ACPI_6_5_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE:
    Status = UpdateSrat ((EFI_ACPI_STATIC_RESOURCE_AFFINITY_TABLE *) TableHeader);
    break;

  case EFI_ACPI_6_5_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE:
    Status = UpdateSlit (TableHeader);
    break;
  }
  return Status;
}
