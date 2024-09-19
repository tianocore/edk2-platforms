/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AC01_PCIE_LIB_H_
#define AC01_PCIE_LIB_H_

/**
  Setup and initialize the AC01 PCIe Root Complex and underneath PCIe controllers

  @param RootComplex           Pointer to Root Complex structure
  @param ReInit                Re-init status
  @param ReInitPcieIndex       PCIe controller index

  @retval RETURN_SUCCESS       The Root Complex has been initialized successfully.
  @retval RETURN_DEVICE_ERROR  PHY, Memory or PIPE is not ready.
**/
RETURN_STATUS
Ac01PcieCoreSetupRC (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN BOOLEAN            ReInit,
  IN UINT8              ReInitPcieIndex
  );

/**
  Verify the link status and retry to initialize the Root Complex if there's any issue.

  @param RootComplexList      Pointer to the Root Complex list
**/
VOID
Ac01PcieCorePostSetupRC (
  IN AC01_ROOT_COMPLEX  *RootComplexList
  );

/**
  Callback function when the Host Bridge enumeration end.

  @param RootComplex          Pointer to the Root Complex structure
**/
VOID
Ac01PcieCoreEndEnumeration (
  IN AC01_ROOT_COMPLEX  *RootComplex
  );

#endif /* AC01_PCIE_LIB_H_ */
