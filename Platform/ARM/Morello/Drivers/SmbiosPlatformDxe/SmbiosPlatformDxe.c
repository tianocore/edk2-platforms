/** @file
  Install SMBIOS tables for Arm Morello System Development Platform.

  This file is the driver entry point for installing SMBIOS tables on Arm
  Morello System Development Platform. For each SMBIOS table installation
  handler registered, the driver invokes the handler to register
  the respective table.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0
**/

#include <IndustryStandard/SmBios.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <PiDxe.h>
#include <Protocol/Smbios.h>
#include <MorelloPlatform.h>

#include "SmbiosPlatformDxe.h"

typedef EFI_STATUS (*ARM_MORELLO_SMBIOS_TABLE_INSTALL_FPTR)(
  EFI_SMBIOS_PROTOCOL *
  );

STATIC CONST
ARM_MORELLO_SMBIOS_TABLE_INSTALL_FPTR  mSmbiosTableList[] = {
  &InstallType0BiosInformation,
  &InstallType1SystemInformation,
  &InstallType3SystemEnclosure,
  &InstallType4ProcessorInformation,
  &InstallType7CacheInformation,
  &InstallType16PhysicalMemoryArray,
  &InstallType17MemoryDevice,
  &InstallType19MemoryArrayMappedAddress,
  &InstallType32SystemBootInformation,
};

/**
  Determine the Dram Block2 Size

  Determine the Dram Block2 Size  by using the data in the Platform
  ID Descriptor HOB to lookup for a matching Dram Block2 Size.

  @retval EFI_SUCCESS    Identified Dram Block2 size on the platform.
  @retval EFI_NOT_FOUND  Failed to identify Dram Block2 Size.
**/
EFI_STATUS
MorelloGetDramBlock2Size (
  UINT64  *DramBlock2Size
  )
{
  VOID                   *SystemIdHob;
  MORELLO_PLAT_INFO_SOC  *HobData;

  SystemIdHob = GetFirstGuidHob (&gArmMorelloPlatformInfoDescriptorGuid);
  if (SystemIdHob == NULL) {
    return EFI_NOT_FOUND;
  }

  HobData = (MORELLO_PLAT_INFO_SOC *)GET_GUID_HOB_DATA (SystemIdHob);

  if (HobData->LocalDdrSize > MORELLO_DRAM_BLOCK1_SIZE) {
    *DramBlock2Size = HobData->LocalDdrSize - MORELLO_DRAM_BLOCK1_SIZE;
  }

  return EFI_SUCCESS;
}

/**
  Driver entry point. Installs SMBIOS information.

  For all the available SMBIOS table installation handlers, invoke each of
  those handlers and let the handlers install the SMBIOS tables. The count
  of handlers that fail to install the SMBIOS tables is maintained for
  additional logging.

  @param ImageHandle     Module's image handle.
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS    All SMBIOS table install handlers invoked.
  @retval EFI_NOT_FOUND  Unsupported platform.
  @retval Others         Failed to invoke SMBIOS table install handlers.
**/
EFI_STATUS
EFIAPI
SmbiosTableEntryPoint (
  IN     EFI_HANDLE        ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_PROTOCOL  *Smbios   = NULL;
  UINT8                CountFail = 0;
  UINT8                Idx;

  /* Find the SMBIOS protocol */
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to install SMBIOS: Unable to locate protocol\n"
      ));
    return Status;
  }

  /* Install the tables listed in mSmbiosTableList */
  for (Idx = 0; Idx < ARRAY_SIZE (mSmbiosTableList); Idx++) {
    Status = (*mSmbiosTableList[Idx])(Smbios);
    if (EFI_ERROR (Status)) {
      CountFail++;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "Installed %d of %d available SMBIOS tables\n",
    ARRAY_SIZE (mSmbiosTableList) - CountFail,
    ARRAY_SIZE (mSmbiosTableList)
    ));

  return EFI_SUCCESS;
}
