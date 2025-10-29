/** @file
  SMBIOS Type 45 (Firmware Inventory Information) table for ARM Morello System
  Development Platform.

  This file installs SMBIOS Type 45 (Firmware Inventory Information) table for
  ARM Morello System Development Platform. It includes information about the
  firmware components in the system.

  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.5.0, Chapter 7.46
**/

#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Protocol/Smbios.h>

#include <MorelloPlatform.h>

#include "SmbiosPlatformDxe.h"

#define FW_VERSION_SIZE  15
#define FW_ID_SIZE       9
#define FW_INFO_SIZE     (FW_VERSION_SIZE + FW_ID_SIZE)

#define FW_DEFAULT_INFO_STRING  "unknown_00000000"

#define TYPE45_STRINGS_EDK2                                     \
  "EDK II\0"                 /* Firmware Name */                \
  "ARM LTD\0"                /* Manufacturer Name */            \
  "N/A\0"                    /* Lowest supported fw version */  \
  "N/A\0"                    /* Release Date */

#define TYPE45_STRINGS_EDK2_PLAT                                \
  "EDK II platforms\0"       /* Firmware Name */                \
  "ARM LTD\0"                /* Manufacturer Name */            \
  "N/A\0"                    /* Lowest supported fw version */  \
  "N/A\0"                    /* Release Date */

#define TYPE45_STRINGS_MCC                                      \
  "MCC Firmware\0"           /* Firmware Name */                \
  "ARM LTD\0"                /* Manufacturer Name */            \
  "N/A\0"                    /* Lowest supported fw version */  \
  "N/A\0"                    /* Release Date */

#define TYPE45_STRINGS_PCC                                      \
  "PCC Firmware\0"           /* Firmware Name */                \
  "ARM LTD\0"                /* Manufacturer Name */            \
  "N/A\0"                    /* Lowest supported fw version */  \
  "N/A\0"                    /* Release Date */

#define TYPE45_STRINGS_SCP                                      \
  "SCP Firmware\0"           /* Firmware Name */                \
  "ARM LTD\0"                /* Manufacturer Name */            \
  "N/A\0"                    /* Lowest supported fw version */  \
  "N/A\0"                    /* Release Date */

#define TYPE45_STRINGS_TFA                                      \
  "Trusted Firmware-A\0"     /* Firmware Name */                \
  "ARM LTD\0"                /* Manufacturer Name */            \
  "N/A\0"                    /* Lowest supported fw version */  \
  "N/A\0"                    /* Release Date */

typedef enum {
  FirmwareComponentName = 1,
  ManufacturerName,
  LowestSupportedVersion,
  ReleaseDate,
  FirmwareVersion,
  FirmwareID
} TYPE45_STRING_ELEMENTS;

/* SMBIOS Type45 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE45    Base;
  CHAR8                  Strings[7];
} ARM_MORELLO_SMBIOS_TYPE45;
#pragma pack()

STATIC ARM_MORELLO_SMBIOS_TYPE45  mArmMorelloSmbiosTableCommon = {
  {
    {
      // SMBIOS header
      SMBIOS_TYPE_FIRMWARE_INVENTORY_INFORMATION, // Type 45
      sizeof (SMBIOS_TABLE_TYPE45),               // Length
      SMBIOS_HANDLE_PI_RESERVED
    },
    FirmwareComponentName,          // Firmware Component
    FirmwareVersion,                // Firmware Version
    0x00,                           // Implementation specific format
    FirmwareID,                     // Firmware ID
    0x00,                           // Implementation specific format
    ReleaseDate,                    // Commit date
    ManufacturerName,               // Manufacturer
    LowestSupportedVersion,         // Minimum version supported
    0xFFFFFFFFFFFFFFFF,             // Firmware size, unknown
    { 0b0, 0b1, 0 },                // Characteristics, write-protected
    0x04,                           // State, enabled
    0,                              // Number of associated components
  },
  "\0\0\0\0\0\0"                    // Default empty strings
};

/**
  Allocate memory for a Type 45 SMBIOS table.

  @param[out] SmbiosType45Table   Type 45 SMBIOS Table for the component
  @param[out] SmbiosHandle        Handle for the created SMBIOS table
  @param[out] AdditionalStrStart  Position to insert dynamic component info
  @param[in]  FixedStrings        Strings containing fixed component info
  @param[in]  FixedStringsLength  Length of FixedStrings

  @retval EFI_SUCCESS           Successfully allocated SMBIOS table
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory for the table
**/
STATIC
EFI_STATUS
AllocateSmbiosTable (
  OUT ARM_MORELLO_SMBIOS_TYPE45  **SmbiosType45Table,
  OUT SMBIOS_HANDLE              *SmbiosHandle,
  OUT CHAR8                      **AdditionalStringStart,
  IN CONST CHAR8                 *FixedStrings,
  IN UINT64                      FixedStringsLength
  )
{
  UINT32  Size;

  Size               = FixedStringsLength + sizeof (SMBIOS_TABLE_TYPE45) + FW_INFO_SIZE;
  *SmbiosType45Table = AllocateZeroPool (Size);
  if (*SmbiosType45Table == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (
    *SmbiosType45Table,
    &mArmMorelloSmbiosTableCommon,
    sizeof (SMBIOS_TABLE_TYPE45)
    );

  *SmbiosHandle = ((SMBIOS_STRUCTURE *)(*SmbiosType45Table))->Handle;
  CopyMem ((*SmbiosType45Table)->Strings, FixedStrings, FixedStringsLength);
  *AdditionalStringStart = (*SmbiosType45Table)->Strings + FixedStringsLength - 1;

  return EFI_SUCCESS;
}

/**
  Install SMBIOS table for a component.

  @param[in] Smbios         SMBIOS protocol.
  @param[in] SmbiosType45   SMBIOS table to install.
  @param[in] SmbiosHandle   Handle for the SMBIOS table.

  @retval EFI_SUCCESS           Successfully installed SMBIOS table.
  @retval EFI_NOT_FOUND         Unknown product id.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
STATIC
EFI_STATUS
InstallSmbiosTable (
  IN EFI_SMBIOS_PROTOCOL        *Smbios,
  IN ARM_MORELLO_SMBIOS_TYPE45  *SmbiosType45,
  IN SMBIOS_HANDLE              *SmbiosHandle
  )
{
  EFI_STATUS  Status;

  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     SmbiosHandle,
                     (SMBIOS_STRUCTURE *)SmbiosType45
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to install Type45 SMBIOS table.\n"
      ));
  }

  return Status;
}

/**
  Set version strings in the buffer from string input.

  @param[in, out] Buffer   Output buffer to write into.
  @param[in]      Version  Version and commit string.

  @retval EFI_SUCCESS          Successfully wrote version strings into buffer.
  @retval EFI_BUFFER_TOO_SMALL Version components exceeded the buffer size.
 */
EFI_STATUS
SetVersionFromString (
  IN OUT CHAR8  *Buffer,
  IN     CHAR8  *VersionString
  )
{
  UINT32  Pos;

  for (Pos = 0; VersionString[Pos] != '\0'; Pos++) {
    if (VersionString[Pos] == '_') {
      VersionString[Pos] = '\0';
    }
  }

  if (Pos > FW_INFO_SIZE) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Buffer, VersionString, Pos);

  return EFI_SUCCESS;
}

/**
  Set version strings in the buffer from compressed format.

  @param[in, out] Buffer   Output buffer to write into.
  @param[in]      Version  Version information.
  @param[in]      CommitID Commit ID for the component.

  @retval EFI_SUCCESS          Successfully wrote version strings into buffer.
  @retval EFI_BUFFER_TOO_SMALL Version components exceeded the buffer size.
 **/
EFI_STATUS
SetVersionFromCompressed (
  IN OUT CHAR8   *Buffer,
  IN     UINT32  Version,
  IN     UINT32  CommitID
  )
{
  UINT32  Length;
  UINT32  BufferSize = FW_INFO_SIZE;

  // Set Version, if 0 then declare unknown base version.
  if ((Version >> MORELLO_FW_REVISION_FLAGS_OFFSET) &
      MORELLO_FW_REVISION_MAINLINE_MASK)
  {
    Length = AsciiSPrint (Buffer, BufferSize, "master");
  } else if ((Version >> MORELLO_FW_REVISION_PATCH_OFFSET) == 0) {
    Length = AsciiSPrint (Buffer, BufferSize, "unknown");
  } else {
    Length = AsciiSPrint (
               Buffer,
               BufferSize,
               "%d.%d.%d",
               ((Version >> MORELLO_FW_REVISION_MAJOR_OFFSET) & MORELLO_FW_REVISION_MASK),
               ((Version >> MORELLO_FW_REVISION_MINOR_OFFSET) & MORELLO_FW_REVISION_MASK),
               ((Version >> MORELLO_FW_REVISION_PATCH_OFFSET) & MORELLO_FW_REVISION_MASK)
               );
  }

  // Add if tree is dirty
  if ((Version >> MORELLO_FW_REVISION_FLAGS_OFFSET) & MORELLO_FW_REVISION_DIRTY_MASK) {
    Length += AsciiSPrint (Buffer+Length, BufferSize-Length, "-dirty");
  }

  if (Length > FW_VERSION_SIZE) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Length++;
  BufferSize -= Length;
  Buffer     += Length;

  // Add commit ID entry
  Length = AsciiSPrint (
             Buffer,
             BufferSize,
             "%x",
             CommitID
             ) + 1;
  if (Length > FW_ID_SIZE) {
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}

/**
  Generate version strings for MCC/PCC.

  @param[in, out] Buffer     Output buffer to write into.
  @param[in]      Version    Version number, in BCD (byte per digit).

  @retval EFI_SUCCESS          Successfully wrote version strings into buffer.
  @retval EFI_BUFFER_TOO_SMALL Version components exceeded the buffer size.
 */
STATIC
EFI_STATUS
SetVersionStringMCCPCC (
  IN OUT CHAR8  *Buffer,
  IN    UINT32  Version
  )
{
  UINT32  Length;

  Length = AsciiSPrint (
             Buffer,
             FW_VERSION_SIZE,
             "v%d",
             (
              (((Version >> MORELLO_MCC_PCC_UPPER_OFFSET) & MORELLO_MCC_PCC_DIGIT_MASK) * 100) +
              (((Version >> MORELLO_MCC_PCC_MID_OFFSET) & MORELLO_MCC_PCC_DIGIT_MASK) * 10) +
              ((Version >> MORELLO_MCC_PCC_LOWER_OFFSET) & MORELLO_MCC_PCC_DIGIT_MASK)
             )
             );
  Buffer += (Length + 1);
  AsciiSPrint (Buffer, FW_ID_SIZE, "N/A");
  return EFI_SUCCESS;
}

/**
  Install SMBIOS firmware inventory information

  Install the SMBIOS firmware inventory information (type 45) table for ARM Morello
  System Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Records were added.
  @retval EFI_OUT_OF_RESOURCES  Records were not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType45FirmwareInventoryInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                    Status;
  SMBIOS_HANDLE                 SmbiosHandle;
  VOID                          *SystemIdFirmware;
  CONST MORELLO_FW_VERSION_SOC  *FwVersion;
  ARM_MORELLO_SMBIOS_TYPE45     *SmbiosType45       = NULL;
  CHAR8                         *AdditionalStrStart = NULL;
  CHAR16                        *EdkVersionString;
  CHAR8                         *EdkFwVersion     = NULL;
  CHAR8                         *EdkPlatFwVersion = NULL;
  UINT8                         Pos;

  // Get EDK Version strings and create ASCII strings for EDK II and EDK II platforms
  EdkVersionString = (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString);

  if (EdkVersionString != NULL) {
    for ( Pos = 0; EdkVersionString[Pos] != ':' &&
          EdkVersionString[Pos] != '\0'; Pos++ )
    {
    }

    if (EdkVersionString[Pos] != '\0') {
      EdkFwVersion = AllocateZeroPool (Pos+1);
      if (EdkFwVersion == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      AsciiSPrint (EdkFwVersion, Pos+1, "%s", EdkVersionString);
      EdkFwVersion[Pos] = '\0';

      for ( EdkVersionString += Pos + 1, Pos = 0; EdkVersionString[Pos] != '\0'; Pos++ ) {
      }

      EdkPlatFwVersion = AllocateZeroPool (Pos+1);
      if (EdkPlatFwVersion == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      AsciiSPrint (EdkPlatFwVersion, Pos+1, "%s", EdkVersionString);
    }
  }

  if (EdkFwVersion == NULL) {
    EdkFwVersion = AllocateZeroPool (sizeof (FW_DEFAULT_INFO_STRING) + 1);
    if (EdkFwVersion == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AsciiSPrint (EdkFwVersion, sizeof (FW_DEFAULT_INFO_STRING) + 1, FW_DEFAULT_INFO_STRING);
  }

  if (EdkPlatFwVersion == NULL) {
    EdkPlatFwVersion = AllocateZeroPool (sizeof (FW_DEFAULT_INFO_STRING) + 1);
    if (EdkPlatFwVersion == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AsciiSPrint (EdkPlatFwVersion, sizeof (FW_DEFAULT_INFO_STRING) + 1, FW_DEFAULT_INFO_STRING);
  }

  /* Handle EDK II */
  AllocateSmbiosTable (
    &SmbiosType45,
    &SmbiosHandle,
    &AdditionalStrStart,
    TYPE45_STRINGS_EDK2,
    sizeof (TYPE45_STRINGS_EDK2)
    );

  Status = SetVersionFromString (AdditionalStrStart, EdkFwVersion);
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to load EDK II version data for Type45 SMBIOS table.\n"
      ));
    return Status;
  }

  Status = InstallSmbiosTable (Smbios, SmbiosType45, &SmbiosHandle);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  /* Handle EDK II platforms */
  AllocateSmbiosTable (
    &SmbiosType45,
    &SmbiosHandle,
    &AdditionalStrStart,
    TYPE45_STRINGS_EDK2_PLAT,
    sizeof (TYPE45_STRINGS_EDK2_PLAT)
    );

  Status = SetVersionFromString (AdditionalStrStart, EdkPlatFwVersion);
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to load EDK II platforms data for Type45 SMBIOS table.\n"
      ));
    return Status;
  }

  Status = InstallSmbiosTable (Smbios, SmbiosType45, &SmbiosHandle);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  // Fetch information for other components (PCC, MCC, SCP, TF-A)
  SystemIdFirmware = GetFirstGuidHob (&gArmMorelloFirmwareVersionGuid);
  if (SystemIdFirmware == NULL) {
    return EFI_NOT_FOUND;
  }

  FwVersion = (MORELLO_FW_VERSION_SOC *)GET_GUID_HOB_DATA (SystemIdFirmware);

  /* Handle MCC */
  AllocateSmbiosTable (
    &SmbiosType45,
    &SmbiosHandle,
    &AdditionalStrStart,
    TYPE45_STRINGS_MCC,
    sizeof (TYPE45_STRINGS_MCC)
    );

  Status = SetVersionStringMCCPCC (AdditionalStrStart, FwVersion->MccFwRevision);
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to load MCC version data for Type45 SMBIOS table.\n"
      ));
  }

  Status = InstallSmbiosTable (Smbios, SmbiosType45, &SmbiosHandle);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  /* Handle PCC */
  AllocateSmbiosTable (
    &SmbiosType45,
    &SmbiosHandle,
    &AdditionalStrStart,
    TYPE45_STRINGS_PCC,
    sizeof (TYPE45_STRINGS_PCC)
    );

  Status = SetVersionStringMCCPCC (AdditionalStrStart, FwVersion->PccFwRevision);
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to load PCC version data for Type45 SMBIOS table.\n"
      ));
  }

  Status = InstallSmbiosTable (Smbios, SmbiosType45, &SmbiosHandle);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  /* Handle SCP Firmware */
  AllocateSmbiosTable (
    &SmbiosType45,
    &SmbiosHandle,
    &AdditionalStrStart,
    TYPE45_STRINGS_SCP,
    sizeof (TYPE45_STRINGS_SCP)
    );

  Status = SetVersionFromCompressed (
             AdditionalStrStart,
             FwVersion->ScpFwRevision,
             FwVersion->ScpFwCommit
             );
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to load SCP Firmware version data for Type45 SMBIOS table.\n"
      ));
    return Status;
  }

  Status = InstallSmbiosTable (Smbios, SmbiosType45, &SmbiosHandle);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  /* Handle TF-A */
  AllocateSmbiosTable (
    &SmbiosType45,
    &SmbiosHandle,
    &AdditionalStrStart,
    TYPE45_STRINGS_TFA,
    sizeof (TYPE45_STRINGS_TFA)
    );

  Status = SetVersionFromString (AdditionalStrStart, (CHAR8 *)FwVersion->TfFwRevision);
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to load TF-A version data for Type45 SMBIOS table.\n"
      ));
    return Status;
  }

  Status = InstallSmbiosTable (Smbios, SmbiosType45, &SmbiosHandle);

  return Status;
}
