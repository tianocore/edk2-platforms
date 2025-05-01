/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NBIOCOMMONLIBDXE_H_
#define NBIOCOMMONLIBDXE_H_

/**
  Function to retrieve SOC_LOGICAL_ID

  @param[out]  LogicalId         Pointer to SOC_LOGICAL_ID
  @retval      EFI_UNSUPPORTED

**/
EFI_STATUS
PcieGetLogicalId (
  OUT   SOC_LOGICAL_ID  *LogicalId
  );

/**
  Function to retrieve PCIE_PLATFORM_CONFIG

  @param[out]  Pcie            Pointer to PCIE_PLATFORM_CONFIG Pointer
  @retval      EFI_UNSUPPORTED

**/
EFI_STATUS
PcieGetPcieDxe (
  OUT     PCIE_PLATFORM_CONFIG  **Pcie
  );

#endif // NBIOCOMMONLIBDXE_H_
