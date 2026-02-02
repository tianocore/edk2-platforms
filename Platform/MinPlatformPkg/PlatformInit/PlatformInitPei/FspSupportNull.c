/** @file FspSupportNull.c
  Provides dummy FSP mode selection value that will always be 0.

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
  return 0;
}
