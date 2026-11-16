/** @file
  SMBIOS Type 0 (BIOS Information) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE0  mSmbiosType0Template = {
  { EFI_SMBIOS_TYPE_BIOS_INFORMATION, sizeof (SMBIOS_TABLE_TYPE0), 0 },
  1,          // Vendor String
  2,          // BiosVersion String
  0xE000,     // BiosSegment
  3,          // BiosReleaseDate String
  0xFF,       // BiosSize -> FFh - size is 16MB or greater, see ExtendedBiosSize
  {       // BiosCharacteristics
    0,    //  Reserved                          :2;  ///< Bits 0-1.
    0,    //  Unknown                           :1;
    0,    //  BiosCharacteristicsNotSupported   :1;
    0,    //  IsaIsSupported                    :1;
    0,    //  McaIsSupported                    :1;
    0,    //  EisaIsSupported                   :1;
    1,    //  PciIsSupported                    :1;
    0,    //  PcmciaIsSupported                 :1;
    0,    //  PlugAndPlayIsSupported            :1;
    0,    //  ApmIsSupported                    :1;
    1,    //  BiosIsUpgradable                  :1;
    0,    //  BiosShadowingAllowed              :1;
    0,    //  VlVesaIsSupported                 :1;
    0,    //  EscdSupportIsAvailable            :1;
    0,    //  BootFromCdIsSupported             :1;
    1,    //  SelectableBootIsSupported         :1;
    0,    //  RomBiosIsSocketed                 :1;
    0,    //  BootFromPcmciaIsSupported         :1;
    0,    //  EDDSpecificationIsSupported       :1;
    0,    //  JapaneseNecFloppyIsSupported      :1;
    0,    //  JapaneseToshibaFloppyIsSupported  :1;
    0,    //  Floppy525_360IsSupported          :1;
    0,    //  Floppy525_12IsSupported           :1;
    0,    //  Floppy35_720IsSupported           :1;
    0,    //  Floppy35_288IsSupported           :1;
    0,    //  PrintScreenIsSupported            :1;
    0,    //  Keyboard8042IsSupported           :1;
    0,    //  SerialIsSupported                 :1;
    0,    //  PrinterIsSupported                :1;
    0,    //  CgaMonoIsSupported                :1;
    0,    //  NecPc98                           :1;
    0     //  ReservedForVendor                 :32; ///< Bits 32-47 reserved for BIOS vendor
  },
  {       // BIOSCharacteristicsExtensionBytes[]
    0x01, //  AcpiIsSupported                   :1;
    0x08, //  BiosBootSpecIsSupported           :1;
  },
  0x01,                                    // SystemBiosMajorRelease - hardcoded to 1.0 (OEM may adjust)
  0x00,                                    // SystemBiosMinorRelease - hardcoded to 1.0 (OEM may adjust)
  0xFF,                                    // EmbeddedControllerFirmwareMajorRelease
  0xFF,                                    // EmbeddedControllerFirmwareMinorRelease
  { 0,                                0 }, // ExtendedBiosSize (WORD) supported with SMBIOS 3.1.0
};

STATIC CHAR8  *mSmbiosType0Strings[] = {
  "Marvell",               // Vendor String
  "1.0",                   // BiosVersion String
  __DATE__,                // BiosReleaseDate String
  NULL
};

/**
  Install the SMBIOS Type 0 (BIOS Information) record.

  Sets the BIOS version and release date strings from the driver context and
  encodes the flash device size in BiosSize, or in ExtendedBiosSize when the
  image is 16 MB or larger.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with BIOS strings and flash size.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType0BiosInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  UINT64  BiosRomSize;

  BiosRomSize = FixedPcdGet64 (PcdFlashDeviceSize);

  SMBIOS_SET_STRING (mSmbiosType0Strings, &mSmbiosType0Template, BiosVersion, Context->BiosVer);
  SMBIOS_SET_STRING (mSmbiosType0Strings, &mSmbiosType0Template, BiosReleaseDate, Context->ReleaseDate);

  if (BiosRomSize < SIZE_16MB) {
    // 64K * (n+1) bytes
    mSmbiosType0Template.BiosSize = (UINT8)DivU64x32 (BiosRomSize, 64 * 1024) - 1;
  } else {
    // mSmbiosType0Template.BiosSize is preset with 0xFF, adjust ExtendedBiosSize.
    mSmbiosType0Template.ExtendedBiosSize.Unit = (BiosRomSize >= SIZE_1GB) ? 0x1 : 0x0; // 00b - megabytes, 01b - gigabytes
    mSmbiosType0Template.ExtendedBiosSize.Size =
      (UINT16)(BiosRomSize / ((BiosRomSize >= SIZE_1GB) ? SIZE_1GB : SIZE_1MB));
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: BIOS Size = 0x%lx, Bios Version: %a, Bios Release Date: %a\n",
    __FUNCTION__,
    BiosRomSize,
    Context->BiosVer,
    Context->ReleaseDate
    ));

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType0Template,
           mSmbiosType0Strings,
           NULL
           );
}
