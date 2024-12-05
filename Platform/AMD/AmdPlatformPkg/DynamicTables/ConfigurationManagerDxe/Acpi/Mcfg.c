/** @file
  This file contains the implementation of the MCFG table initialization for the
  Configuration Manager DXE driver.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/PciSegmentInfoLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../ConfigurationManager.h"

/** Updates ACPI MCFG table information in the platform repository.

  @param [in]  PlatformRepo  Pointer to the platform repository.

  @retval EFI_SUCCESS        The ACPI MCFG table information is updated.
  @retval EFI_INVALID_PARAMETER  The input parameter is invalid.
  @retval EFI_NOT_FOUND      The PCI segment information is not found.
**/
EFI_STATUS
EFIAPI
UpdateMcfgTableInfo (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  )
{
  PCI_SEGMENT_INFO                      *PciSegmentInfo;
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO  *PciConfigSpaceInfo;
  UINTN                                 PciSegmentCount;
  UINTN                                 Index;

  if (PlatformRepo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PciSegmentInfo  = NULL;
  PciSegmentCount = 0;
  PciSegmentInfo  = GetPciSegmentInfo (&PciSegmentCount);
  if ((PciSegmentInfo == NULL) || (PciSegmentCount == 0)) {
    return EFI_NOT_FOUND;
  }

  PciConfigSpaceInfo = AllocateZeroPool (sizeof (CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO) * PciSegmentCount);
  if (PciConfigSpaceInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < PciSegmentCount; Index++) {
    PciConfigSpaceInfo[Index].BaseAddress           = PciSegmentInfo[Index].BaseAddress;
    PciConfigSpaceInfo[Index].PciSegmentGroupNumber = PciSegmentInfo[Index].SegmentNumber;
    PciConfigSpaceInfo[Index].StartBusNumber        = PciSegmentInfo[Index].StartBusNumber;
    PciConfigSpaceInfo[Index].EndBusNumber          = PciSegmentInfo[Index].EndBusNumber;
  }

  PlatformRepo->PciConfigSpaceInfoCount = PciSegmentCount;
  PlatformRepo->PciConfigSpaceInfo      = PciConfigSpaceInfo;

  return EFI_SUCCESS;
}
