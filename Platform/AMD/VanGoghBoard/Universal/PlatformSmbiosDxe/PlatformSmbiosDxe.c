/** @file
  Static SMBIOS Table for platform

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2012, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/SmbiosLib.h>
#include <Library/HobLib.h>

extern SMBIOS_TEMPLATE_ENTRY  gSmbiosTemplate[];

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
PlatformSmbiosDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  SMBIOS_STRUCTURE_POINTER  Smbios;

  DEBUG ((DEBUG_INFO, " PlatfomrSmbiosDriverEntryPoint \n"));

  // Phase 0 - Patch table to make SMBIOS 2.7 structures smaller to conform
  //           to an early version of the specification.

  // Phase 1 - Initialize SMBIOS tables from template
  Status = SmbiosLibInitializeFromTemplate (gSmbiosTemplate);
  ASSERT_EFI_ERROR (Status);

  // Phase 2 - Patch SMBIOS table entries
  Smbios.Hdr = SmbiosLibGetRecord (EFI_SMBIOS_TYPE_BIOS_INFORMATION, 0, &SmbiosHandle);
  if (Smbios.Type0 != NULL) {
    // 64K * (n+1) bytes
    Smbios.Type0->BiosSize = (UINT8)DivU64x32 (FixedPcdGet64 (PcdFlashAreaSize), 64*1024) - 1;

    SmbiosLibUpdateUnicodeString (
      SmbiosHandle,
      Smbios.Type0->BiosVersion,
      (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString)
      );

    DEBUG ((
      DEBUG_INFO,
      " Smbios.Type0->BiosSize: %dMB, Smbios.Type0->BiosVersion: %S, Build Time: %a,%a\n",
      (Smbios.Type0->BiosSize +1) * 64 / 1024,
      (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString),
      __DATE__,
      __TIME__
      ));
  }

  return EFI_SUCCESS;
}
