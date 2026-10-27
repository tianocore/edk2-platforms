/** @file

  Source file for the CN10K platform DXE driver.

  Copyright (c) 2015-2016, Cavium inc. All rights reserved.<BR>
  Copyright (c) 2026, Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/EventGroup.h>

#include <IndustryStandard/SmcLib.h>
#include <Library/FdtLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/CN10KConfigLib.h>
#include <Library/UefiBootServicesTableLib.h>

VOID  *mFdtBlobBase = NULL;

/**
  Get the number of CPU nodes.

  @return                The number of CPU nodes.
**/
STATIC INT32
GetNumNodes (
  VOID
  )
{
  CHAR8       String[50];
  UINT32      Index;
  INT32       Offset;

  if (mFdtBlobBase == NULL) {
    return 0;
  }

  Index  = 0;

  do {
    Offset = 0;
    AsciiSPrint (String, sizeof (String), "/cpus/cpu-map/cluster%d", Index);
    Offset = FdtPathOffset (mFdtBlobBase, String);
    if (Offset > 0) {
      Index++;
    }
  } while (Offset > 0);

  return Index;
}

/**
  Get the number of CPU cores on a given CPU node.

  @param[in]  NodeNum    The CPU node number.

  @return                The number of CPU cores on that node.
**/
STATIC INT32
GetNumCoresPerNode (
  IN  UINT32  NodeNum
  )
{
  CHAR8       String[50];
  UINT32      Index;
  INT32       Offset;

  if (mFdtBlobBase == NULL) {
    return 0;
  }

  Index  = 0;

  do {
    Offset = 0;
    AsciiSPrint (String, sizeof (String), "/cpus/cpu-map/cluster%d/core%d", NodeNum, Index);
    Offset = FdtPathOffset (mFdtBlobBase, String);
    if (Offset > 0) {
      Index++;
    }
  } while (Offset > 0);

  return Index;
}

/**
  Setup CPU PCDs.

  @retval  EFI_SUCCESS  On success. No error code defined.
**/
STATIC EFI_STATUS
CN10KPlatformSetupPcd (
  VOID
  )
{
  CHAR8   BoardName[32];
  INT32   PcdValue;

  CN10KGetConfigPropertyAscii ("BOARD-MODEL", BoardName, sizeof (BoardName));

  DEBUG ((DEBUG_INFO, "Board: %a\n", BoardName));

  PcdValue = GetNumNodes ();
  if (PcdValue > 0) {
    PcdSet32S (PcdNodeCount, PcdValue);
  }

  PcdValue = GetNumCoresPerNode (0);
  if (PcdValue > 0) {
    PcdSet32S (PcdCoresPerNode, PcdValue);
  }

  return EFI_SUCCESS;
}

/**
  CN10K platform DXE entry point.

  @param[in]  ImageHandle  A handle to the image.
  @param[in]  SystemTable  A pointer to the EFI system table.

  @retval  EFI_SUCCESS  On success. No error code defined.
**/
EFI_STATUS
EFIAPI
CN10KPlatformDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mFdtBlobBase = (VOID *)CN10KGetFdt ();
  CN10KPlatformSetupPcd ();
  SmcBootSuccess ();
  return EFI_SUCCESS;
}
