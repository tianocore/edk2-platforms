/** @file
  VExpress library for ARM's WatchdogDxe

  Copyright (c) 2025 Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define VEXPRESS_GICV5_WATCHDOG_INTID  0x6000001b

RETURN_STATUS
EFIAPI
ArmVExpressWatchdogLibConstructor (
  VOID
  )
{
  RETURN_STATUS  PcdStatus;

  if (ArmHasGicV5SystemRegisters ()) {
    PcdStatus = PcdSet32S (PcdGenericWatchdogEl2IntrNum, VEXPRESS_GICV5_WATCHDOG_INTID);
    ASSERT_RETURN_ERROR (PcdStatus);
  }

  return EFI_SUCCESS;
}
