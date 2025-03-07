/** @file

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/PciIo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <IndustryStandard/Pci22.h>

/**
* This function installs a protocol used by platform BIOS to provide the hotplug descriptor.
*
*  @param[in]  ImageHandle        Image handler
*  @param[in]  SystemTable        Pointer to the system table
*
*  @retval EFI_SUCCESS     The thread was successfully launched.
*
**/
EFI_STATUS
EFIAPI
OobPprEntry (
  IN       EFI_HANDLE        ImageHandle,
  IN       EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
