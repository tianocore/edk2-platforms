/** @file FspSupport.c
  Provides FSP mode selection value based on PcdFspModeSelection

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

UINT8
EFIAPI
FspGetModeSelection (
  VOID
  )
{
  return PcdGet8 (PcdFspModeSelection);
}
