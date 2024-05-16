/** @file

SetCacheMtrr library functions.
This library implementation is for AMD processor based platforms.

Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/MtrrLib.h>

/**
  This function sets the cache MTRR values for PEI phase.
**/
VOID
EFIAPI
SetCacheMtrr (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = MtrrSetMemoryAttribute (
             0,
             0xA0000,
             CacheWriteBack
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error(%r) in setting CacheWriteBack for 0-0x9FFFF\n",
      Status
      ));
  }

  Status = MtrrSetMemoryAttribute (
             0xA0000,
             0x20000,
             CacheUncacheable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error(%r) in setting CacheUncacheable for 0xA0000-0xBFFFF\n",
      Status
      ));
  }

  Status = MtrrSetMemoryAttribute (
             0xC0000,
             0x40000,
             CacheWriteProtected
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error(%r) in setting CacheWriteProtected for 0xC0000-0xFFFFF\n",
      Status
      ));
  }

  Status = MtrrSetMemoryAttribute (
             0x100000,
             0xAFF00000,
             CacheWriteBack
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error(%r) in setting CacheWriteBack for 0x100000-0xAFFFFFFF\n",
      Status
      ));
  }

  Status = MtrrSetMemoryAttribute (
             PcdGet32 (PcdFlashAreaBaseAddress),
             PcdGet32 (PcdFlashAreaSize),
             CacheWriteProtected
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error(%r) in setting CacheWriteProtected for 0x%X-0x%X\n",
      Status,
      PcdGet32 (PcdFlashAreaBaseAddress),
      PcdGet32 (PcdFlashAreaBaseAddress) + PcdGet32 (PcdFlashAreaSize)
      ));
  }

  MtrrDebugPrintAllMtrrs ();
  return;
}

/**
  Update MTRR setting in EndOfPei phase.
  This function will set the MTRR value as CacheUncacheable
  for Flash address.

  @retval  EFI_SUCCESS  The function completes successfully.
  @retval  Others       Some error occurs.
**/
EFI_STATUS
EFIAPI
SetCacheMtrrAfterEndOfPei (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = MtrrSetMemoryAttribute (
             PcdGet32 (PcdFlashAreaBaseAddress),
             PcdGet32 (PcdFlashAreaSize),
             CacheUncacheable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error(%r) in setting CacheUncacheable for 0x%X-0x%X\n",
      Status,
      PcdGet32 (PcdFlashAreaBaseAddress),
      PcdGet32 (PcdFlashAreaBaseAddress) + PcdGet32 (PcdFlashAreaSize)
      ));
  }

  MtrrDebugPrintAllMtrrs ();
  return EFI_SUCCESS;
}
