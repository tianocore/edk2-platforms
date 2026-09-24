/** @file
  Platform Device Tree Header.

  Header file for DTB functions used by ArmPlatformLib, providing device tree
  initialization and node access interfaces.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>
#include <Library/DtFrameworkLib.h>

/**
  Get FDT node handle by name.

  @param[in,out]  Node  Pointer to DT node handle.
  @param[in]      Name  Node name to search for.

  @retval  0           Success.
  @retval  Non-zero    FDT error code.

**/
INT32
FdtGetNodeHandle (
  IN OUT DT_NODE_HANDLE  *Node,
  IN     CHAR8           *Name
  );

/**
  Initialize device tree blob.

  Selects appropriate DTB based on chip/platform information and sets up
  the DTB blob for use by the system.

  @retval  EFI_SUCCESS        DTB initialized successfully.
  @retval  EFI_DEVICE_ERROR   Error occurred during DTB initialization.

**/
EFI_STATUS
DtbInit (
  VOID
  );
