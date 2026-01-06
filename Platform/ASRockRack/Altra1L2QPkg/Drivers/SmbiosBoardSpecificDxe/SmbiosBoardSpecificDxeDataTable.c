/** @file
  This file provides SMBIOS Type.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBoardSpecificDxe.h"

SMBIOS_BOARD_SPECIFIC_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE8,
  PlatformPortConnector
  )
SMBIOS_BOARD_SPECIFIC_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE9,
  PlatformSystemSlot
  )
SMBIOS_BOARD_SPECIFIC_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE41,
  PlatformOnboardDevicesExtended
  )

SMBIOS_BOARD_SPECIFIC_DXE_DATA_TABLE mSmbiosBoardSpecificDxeDataTable[] = {
  //Type8
  SMBIOS_BOARD_SPECIFIC_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformPortConnector
  ),
  //Type9
  SMBIOS_BOARD_SPECIFIC_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformSystemSlot
  ),
  //Type41
  SMBIOS_BOARD_SPECIFIC_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformOnboardDevicesExtended
  )
};

//
// Number of Data Table entries.
//
UINTN mSmbiosBoardSpecificDxeDataTableEntries = ARRAY_SIZE (mSmbiosBoardSpecificDxeDataTable);
