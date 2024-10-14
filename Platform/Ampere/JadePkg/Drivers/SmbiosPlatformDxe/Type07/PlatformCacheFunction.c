/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CpuConfigNVDataStruc.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NVParamLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <NVParamDef.h>

#include "SmbiosPlatformDxe.h"

#define MAX_CACHE_LEVEL  2

#define SLC_SIZE(x)    (UINT16)(0x8000 | (((x) * (1 << 20)) / (64 * (1 << 10))))
#define SLC_SIZE_2(x)  (0x80000000 | (((x) * (1 << 20)) / (64 * (1 << 10))))

typedef enum {
  CacheModeWriteThrough = 0,  ///< Cache is write-through
  CacheModeWriteBack,         ///< Cache is write-back
  CacheModeVariesWithAddress, ///< Cache mode varies by address
  CacheModeUnknown,           ///< Cache mode is unknown
  CacheModeMax
} CACHE_OPERATION_MODE;

typedef enum {
  CacheLocationInternal = 0, ///< Cache is internal to the processor
  CacheLocationExternal,     ///< Cache is external to the processor
  CacheLocationReserved,     ///< Reserved
  CacheLocationUnknown,      ///< Cache location is unknown
  CacheLocationMax
} CACHE_LOCATION;

/**
  Checks whether SLC cache is should be displayed or not.

  @retval TRUE                Should be displayed.
          FALSE               Should not be displayed.
**/
BOOLEAN
CheckSlcCache (
  VOID
  )
{
  EFI_STATUS         Status;
  UINT32             NumaMode;
  UINTN              CpuConfigDataSize;
  CPU_VARSTORE_DATA  CpuConfigData;

  CpuConfigDataSize = sizeof (CPU_VARSTORE_DATA);

  Status = gRT->GetVariable (
                  CPU_CONFIG_VARIABLE_NAME,
                  &gCpuConfigFormSetGuid,
                  NULL,
                  &CpuConfigDataSize,
                  &CpuConfigData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can not get CPU configuration information - %r\n", __func__, Status));

    Status = NVParamGet (
               NV_SI_SUBNUMA_MODE,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &NumaMode
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Can not get SubNUMA mode - %r\n", __func__, Status));
      NumaMode = SUBNUMA_MODE_MONOLITHIC;
    }

    if (!IsSlaveSocketActive () && (NumaMode == SUBNUMA_MODE_MONOLITHIC)) {
      return TRUE;
    }
  } else if (CpuConfigData.CpuSlcAsL3 == CPU_SLC_AS_L3_ENABLE) {
    return TRUE;
  }

  return FALSE;
}

/**
  Fills necessary information of SLC in SMBIOS Type 7.

  @param[out] Type7Record                         The Type 7 structure to allocate and initialize.

  @retval     EFI_SUCCESS                         The Type 7 structure was successfully
                                                  allocated and the strings initialized.
              EFI_OUT_OF_RESOURCES                Could not allocate memory needed.
**/
VOID
ConfigSlcArchitectureInformation (
  OUT SMBIOS_TABLE_TYPE7  *InputData
  )
{
  // Cache Size
  if (IsAc01Processor ()) {
    //
    // Altra's SLC size is 32MB
    //
    InputData->MaximumCacheSize  = SLC_SIZE (32);
    InputData->MaximumCacheSize2 = SLC_SIZE_2 (32);
  } else {
    //
    // Altra Max's SLC size is 16MB
    //
    InputData->MaximumCacheSize  = SLC_SIZE (16);
    InputData->MaximumCacheSize2 = SLC_SIZE_2 (16);
  }

  InputData->InstalledSize  = InputData->MaximumCacheSize;
  InputData->InstalledSize2 = InputData->MaximumCacheSize2;
}

/**
  This function adds SMBIOS Table (Type 7) records for System Level Cache (SLC).

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to add the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformCache) {
  EFI_STATUS          Status;
  STR_TOKEN_INFO      *InputStrToken;
  SMBIOS_TABLE_TYPE7  *Type7Record;
  SMBIOS_TABLE_TYPE7  *InputData;

  if (CheckSlcCache ()) {
    InputData     = (SMBIOS_TABLE_TYPE7 *)RecordData;
    InputStrToken = (STR_TOKEN_INFO *)StrToken;

    while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
      ConfigSlcArchitectureInformation (InputData);
      SmbiosPlatformDxeCreateTable (
        (VOID *)&Type7Record,
        (VOID *)&InputData,
        sizeof (SMBIOS_TABLE_TYPE7),
        InputStrToken
        );
      if (Type7Record == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type7Record, NULL);
      if (EFI_ERROR (Status)) {
        FreePool (Type7Record);
        return Status;
      }

      FreePool (Type7Record);
      InputData++;
      InputStrToken++;
    }
  }

  return EFI_SUCCESS;
}
