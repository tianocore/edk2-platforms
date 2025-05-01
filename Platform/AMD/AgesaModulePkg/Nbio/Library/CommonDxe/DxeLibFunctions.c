/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <GnbDxio.h>
#include <Library/AmdBaseLib.h>
#include <Library/PcieConfigLib.h>
#include <Library/NbioCommonLibDxe.h>

/**
  Function to retrieve SOC_LOGICAL_ID

  @param[out]  LogicalId         Pointer to SOC_LOGICAL_ID
  @retval      EFI_UNSUPPORTED

**/
EFI_STATUS
PcieGetLogicalId (
  OUT   SOC_LOGICAL_ID  *LogicalId
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Function to retrieve PCIE_PLATFORM_CONFIG

  @param[out]  Pcie            Pointer to PCIE_PLATFORM_CONFIG Pointer
  @retval      EFI_UNSUPPORTED

**/
EFI_STATUS
PcieGetPcieDxe (
  OUT     PCIE_PLATFORM_CONFIG  **Pcie
  )
{
  return EFI_UNSUPPORTED;
}
