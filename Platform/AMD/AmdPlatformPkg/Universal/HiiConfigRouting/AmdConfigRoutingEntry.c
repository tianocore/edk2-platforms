/** @file
  AMD implementation of interface functions for EFI_HII_CONFIG_ROUTING_PROTOCOL.
  This module overrides BlockToConfig and ConfigToBlock for the better performance.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AmdHiiConfigRouting.h"

/**
  Entry point for the AMD HII Config Routing driver.

  @param[in] ImageHandle  The image handle.
  @param[in] SystemTable  The system table.

  @retval EFI_SUCCESS  The entry point is executed successfully.
  @retval Others       Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
AmdConfigRoutingEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  HiiConfigRouting = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&HiiConfigRouting
                  );
  if (!EFI_ERROR (Status)) {
    ASSERT (HiiConfigRouting != NULL);
    DEBUG ((
      DEBUG_INFO,
      "HiiConfigRouting->BlockToConfig: 0x%lX\n",
      (UINTN)HiiBlockToConfig
      ));
    DEBUG ((
      DEBUG_INFO,
      "HiiConfigRouting->ConfigToBlock: 0x%lX\n",
      (UINTN)HiiConfigToBlock
      ));

    HiiConfigRouting->BlockToConfig = HiiBlockToConfig;
    HiiConfigRouting->ConfigToBlock = HiiConfigToBlock;
  }

  return Status;
}
