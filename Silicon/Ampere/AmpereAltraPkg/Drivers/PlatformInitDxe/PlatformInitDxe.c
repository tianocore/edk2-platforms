/** @file

   Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

   SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Uefi.h>

#include <IndustryStandard/SmBios.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FlashLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>

#include "PlatformInitDxe.h"

BOOLEAN
FailSafeValidCRC (
  IN FAIL_SAFE_CONTEXT  *FailSafeBuf
  )
{
  BOOLEAN  Valid;
  UINT16   Crc;
  UINT32   Len;

  Len                = sizeof (FAIL_SAFE_CONTEXT);
  Crc                = FailSafeBuf->CRC16;
  FailSafeBuf->CRC16 = 0;

  Valid              = (Crc == CalculateCrc16CcittF ((VOID *)FailSafeBuf, Len, 0));
  FailSafeBuf->CRC16 = Crc;

  return Valid;
}

BOOLEAN
FailSafeFailureStatus (
  IN UINT8  Status
  )
{
  if ((Status == FAILSAFE_BOOT_LAST_KNOWN_SETTINGS) ||
      (Status == FAILSAFE_BOOT_DEFAULT_SETTINGS) ||
      (Status == FAILSAFE_BOOT_DDR_DOWNGRADE))
  {
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
FailSafeClearContext (
  VOID
  )
{
  EFI_STATUS         Status;
  FAIL_SAFE_CONTEXT  FailSafeBuf;
  UINT32             FailSafeSize;
  UINT64             FailSafeStartOffset;

  Status = FlashGetFailSafeInfo (&FailSafeStartOffset, &FailSafeSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get context region information\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Status = FlashReadCommand (FailSafeStartOffset, (UINT8 *)&FailSafeBuf, sizeof (FAIL_SAFE_CONTEXT));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If failsafe context is valid, and:
  //    - The status indicate non-failure, then don't clear it
  //    - The status indicate a failure, then go and clear it
  //
  if (  FailSafeValidCRC (&FailSafeBuf)
     && !FailSafeFailureStatus (FailSafeBuf.Status))
  {
    return EFI_SUCCESS;
  }

  return FlashEraseCommand (FailSafeStartOffset, FailSafeSize);
}

EFI_STATUS
EFIAPI
PlatformInitDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = FailSafeClearContext ();
  ASSERT_EFI_ERROR (Status);

  return Status;
}
