/** @file
  The PEI driver for SMRAM space location.

  Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/SmmRelocationLib.h>
#include <Ppi/MpServices2.h>
#include <Guid/SmramMemoryReserve.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <AcpiS3MemReserve.h>

/**
  Notification function called when EDKII_PEI_MP_SERVICES2_PPI becomes available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @retval  EFI_SUCCESS        The status code returned from this function is ignored.

**/
STATIC
EFI_STATUS
EFIAPI
OnMpServices2Available (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );
