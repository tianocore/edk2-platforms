/** @file
  Common Setup Library, to load setup default value.

  Copyright (C) 2015 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/CommonSetupLib.h>

/**
  Load default value for common setup configuration.

  @param[out] CommonSetupOptions Pointer to the structure of COMMON_SETUP_OPTIONS,
                                 this pointer must be allocated with sizeof(COMMON_SETUP_OPTIONS)
                                 before being called.

  @retval EFI_SUCCESS            The common setup options are successfully loaded to default value.

**/
EFI_STATUS
EFIAPI
LoadCommonSetupDefault (
  OUT COMMON_SETUP_OPTIONS  *CommonSetupOptions
  )
{
  DEBUG ((DEBUG_INFO, "%a: - ENTRY\n", __func__));
  //
  // Note: Make sure settings here are in sync with CommonSetupHii.vfr file.
  //
  CommonSetupOptions->AllocatedSize              = COMMON_SETUP_ALLOCATED_SIZE;
  CommonSetupOptions->BiosPasswordEnable         = 0;                                                     // 0: disable, 1: enable
  CommonSetupOptions->BiosPasswordRequest        = 0;                                                     // 0: disable, 1: enable
  CommonSetupOptions->UsbBiosSupport             = 1;                                                     // 0: disable, 1: enable
  CommonSetupOptions->PcdPcieResizableBarSupport = (UINT8)PcdGetBool (PcdPcieResizableBarSupportDefault); // 0: disable, 1: enable
  CommonSetupOptions->NetworkStack               = 1;                                                     // 0: disable, 1: enable
  CommonSetupOptions->KeyboardHotkeyDetect       = 1;                                                     // 0: disable, 1: enable
  CommonSetupOptions->SecureBootControl          = (UINT8)PcdGetBool (PcdEnableSecureBootByDefault);      // 0: disable, 1: enable
  CommonSetupOptions->ApplyDefaultBootOrder      = 0;                                                     // 0: disable, 1: enable
  CommonSetupOptions->NetworkBootFirst           = (UINT8)PcdGetBool (PcdNetworkBootFirstDefault);        // 0: disable, 1: enable
  CommonSetupOptions->NetworkBootSelection       = 0;                                                     // 0: PXE boot, 1: HTTP boot

  if (COMMON_SETUP_MAXIMAL_SIZE != (COMMON_SETUP_ALLOCATED_SIZE + sizeof (CommonSetupOptions->ReservedBuffer))) {
    DEBUG ((DEBUG_ERROR, "COMMON_SETUP_MAXIMAL_SIZE: %d (%X)\n", COMMON_SETUP_MAXIMAL_SIZE, COMMON_SETUP_MAXIMAL_SIZE));
    DEBUG ((
      DEBUG_ERROR,
      "COMMON_SETUP_ALLOCATED_SIZE + sizeof(CommonSetupOptions->ReservedBuffer): %d (%X)\n",
      COMMON_SETUP_ALLOCATED_SIZE + sizeof (CommonSetupOptions->ReservedBuffer),
      COMMON_SETUP_ALLOCATED_SIZE + sizeof (CommonSetupOptions->ReservedBuffer)
      ));
    ASSERT (COMMON_SETUP_MAXIMAL_SIZE == (COMMON_SETUP_ALLOCATED_SIZE + sizeof (CommonSetupOptions->ReservedBuffer)));
  }

  DEBUG ((DEBUG_INFO, "%a: - EXIT\n", __func__));
  return EFI_SUCCESS;
}
