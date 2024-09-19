/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

#define MAX_NODES_PER_SOCKET  4
#define SELF_DISTANCE         10
#define REMOTE_DISTANCE       20

EFI_ACPI_6_3_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER  SLITTableHeaderTemplate = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE,
    0, /* need fill in */
    EFI_ACPI_6_3_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_REVISION
    ),
  0,
};

VOID
ComputeCoordinatesForNode (
  UINTN  Node,
  UINTN  *X,
  UINTN  *Y
  )
{
  switch (Node) {
    case 0:
      *X = 0;
      *Y = 0;
      break;
    case 1:
      *X = 1;
      *Y = 0;
      break;
    case 2:
      *X = 0;
      *Y = 1;
      break;
    case 3:
      *X = 1;
      *Y = 1;
      break;
    default:
      *X = 0;
      *Y = 0;
      break;
  }
}

/**
  Compute the distance between between two nodes on socket.
**/
UINT8
ComputeSlitDistanceOnSocket (
  UINTN  Node1,
  UINTN  Node2
  )
{
  UINTN  X1, Y1, X2, Y2;
  UINTN  XDistance, YDistance;

  ComputeCoordinatesForNode (Node1, &X1, &Y1);
  ComputeCoordinatesForNode (Node2, &X2, &Y2);

  XDistance = ABS ((INTN)(X1 - X2));
  YDistance = ABS ((INTN)(Y1 - Y2));

  return (UINT8)(XDistance + YDistance + SELF_DISTANCE);
}

/**
  Compute the distance between between two nodes on
  different sockets.
  Node1 - local socket node number
  Node2 - remote socket node number
**/
UINT8
ComputeSlitDistanceOnRemoteSocket (
  UINTN  LocalNode,
  UINTN  RemoteNode
  )
{
  UINTN  LocalDistance, RemoteDistance;

  //
  // Mesh forwards traffic between sockets over both CCIX links when going from
  // one quadrant to another. For example, memory access from Node 0 to Node 4
  // results in traffic being split between RCA0 and 1. Hence distance is
  // different only between upper half and lower half of sockets and not
  // between quadrants. Hemisphere configuration is not impacted as there
  // is no upper-half.
  //
  LocalDistance  = 0;
  RemoteDistance = 0;
  if (LocalNode >= (MAX_NODES_PER_SOCKET / 2)) {
    LocalDistance = 1;
  }

  if (RemoteNode >= (MAX_NODES_PER_SOCKET / 2)) {
    RemoteDistance = 1;
  }

  return (UINT8)(LocalDistance + RemoteDistance + REMOTE_DISTANCE);
}

UINT8
ComputeSlitDistance (
  UINTN  Node1,
  UINTN  Node2,
  UINTN  DomainsPerSocket
  )
{
  UINT8  Distance;

  Distance = 0;
  if ((Node1 / DomainsPerSocket) == (Node2 / DomainsPerSocket)) {
    Distance = ComputeSlitDistanceOnSocket (
                 Node1 % DomainsPerSocket,
                 Node2 % DomainsPerSocket
                 );
  } else {
    Distance = ComputeSlitDistanceOnRemoteSocket (
                 Node1 % DomainsPerSocket,
                 Node2 % DomainsPerSocket
                 );
  }

  return Distance;
}

EFI_STATUS
AcpiInstallSlitTable (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL                                         *AcpiTableProtocol;
  EFI_STATUS                                                      Status;
  UINTN                                                           NumDomain, Count, Count1;
  EFI_ACPI_6_3_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER  *SlitTablePointer;
  UINT8                                                           *TmpPtr;
  UINTN                                                           SlitTableKey;
  UINTN                                                           NumDomainPerSocket;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NumDomainPerSocket = CpuGetNumberOfSubNumaRegion ();
  NumDomain          = NumDomainPerSocket * GetNumberOfActiveSockets ();

  SlitTablePointer = (EFI_ACPI_6_3_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER *)
                     AllocateZeroPool (sizeof (SLITTableHeaderTemplate) + NumDomain * NumDomain);
  if (SlitTablePointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem ((VOID *)SlitTablePointer, (VOID *)&SLITTableHeaderTemplate, sizeof (SLITTableHeaderTemplate));
  SlitTablePointer->NumberOfSystemLocalities = NumDomain;
  TmpPtr                                     = (UINT8 *)SlitTablePointer + sizeof (SLITTableHeaderTemplate);
  for (Count = 0; Count < NumDomain; Count++) {
    for (Count1 = 0; Count1 < NumDomain; Count1++, TmpPtr++) {
      *TmpPtr = ComputeSlitDistance (Count, Count1, NumDomainPerSocket);
    }
  }

  SlitTablePointer->Header.Length = sizeof (SLITTableHeaderTemplate) + NumDomain * NumDomain;

  AcpiUpdateChecksum ((UINT8 *)SlitTablePointer, SlitTablePointer->Header.Length);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)SlitTablePointer,
                                SlitTablePointer->Header.Length,
                                &SlitTableKey
                                );
  FreePool ((VOID *)SlitTablePointer);

  return Status;
}
