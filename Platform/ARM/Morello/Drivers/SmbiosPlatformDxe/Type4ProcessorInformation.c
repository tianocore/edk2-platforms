/** @file
  SMBIOS Type 4 (Processor information) table for ARM Morello System
  Development Platform.

  This file installs SMBIOS Type 4 (Processor information) Table for ARM
  Morello System Development Platform. It includes information about
  manufacture, family, processor id, maximum operating frequency, and
  other information related to the processor.

  Copyright (c) 2022 - 2023, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.5
**/

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <MorelloPlatform.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

#define TYPE4_STRINGS                                   \
  "0x000\0"                     /* Part Number */       \
  "ARM LTD\0"                   /* manufacturer */      \
  "Ironwood Socket\0"           /* socket type */       \
  "000-0\0"                     /* Serial number */     \
  "\0"

#define MORELLO_PROCESSOR_VERSION_STR_MAX_LEN  16

typedef enum {
  PartNumber = 1,
  ManufacturerName,
  SocketTypeBase,
  SerialNumberBase,
  ProcessorVersionBase
} TYPE4_STRING_ELEMENTS;

/* SMBIOS Type4 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE4    Base;
  CHAR8                 Strings[sizeof (TYPE4_STRINGS)];
} ARM_MORELLO_SMBIOS_TYPE4;
#pragma pack()

/* Processor information */
STATIC ARM_MORELLO_SMBIOS_TYPE4  mArmMorelloSmbiosType4 = {
  {
    {
      // SMBIOS header
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, // Type 4
      sizeof (SMBIOS_TABLE_TYPE4),           // Length
      SMBIOS_HANDLE_CLUSTER1,                // handle number
    },
    SocketTypeBase,          // Socket type
    CentralProcessor,        // Processor type
    ProcessorFamilyIndicatorFamily2,
    // Use Processor Family 2 field
    ManufacturerName,        // Manufacturer string number
    {
      { 0 }, { 0 }
    },                       // Processor id, update dynamically
    0,                       // Processor version
    {                        // Voltage;// 0x8A (1.0 volts)
      // legacy mode bit 7 set to 1, the remaining seven bits of the field
      // are set to contain the processor’s current voltage times 10.
      0,   // :1;
      1,   // :1;
      0,   // :1;
      1,   // :1;
      0,   // :3;
      1    // :1; bit 7 set to 1.
    },
    50,                      // External clock frequency in MHz
    2500,                    // Max speed in MHz
    2500,                    // Current speed in MHz
    (                        // Status
      (1 << 6) |             // CPU Socket Populated
      (1 << 0)               // CPU Enabled
    ),
    ProcessorUpgradeNone,    // Processor Upgrade
    SMBIOS_HANDLE_L1I_CACHE, // L1 Cache handle
    SMBIOS_HANDLE_L2_CACHE,  // L2 Cache handle
    SMBIOS_HANDLE_L3_CACHE,  // L3 Cache handle
    0,                       // Processor serial number not set
    0,                       // Processor asset tag not set
    PartNumber,              // Part number, update dynamically
    4,                       // Core count
    4,                       // Enabled core count
    4,                       // Thread per socket count
    (                        // Processor characteristics
      (1 << 2) |             // 64-bit Capable
      (1 << 3) |             // Multi-Core
      (1 << 5) |             // Execute Protection
      (1 << 7)               // Power/Performance Control
    ),
    ProcessorFamilyARMv8,    // Processor Family 2
    0,                       // CoreCount2;
    0,                       // EnabledCoreCount2;
    0,                       // ThreadCount2;
  },
  TYPE4_STRINGS
};

/**
  Update the part-number string.

  Get the part number from ProcessorId and update TYPE4_STRINGS

  @param  ProcessorId    The processor Id read from MIDR register
**/
STATIC
VOID
UpdatePartNumber (
  IN     UINT64  ProcessorId
  )
{
  CHAR8   PartNumber[6] = { 0 };
  UINT16  PartNum;

  PartNum = (UINT16)((ProcessorId >> 4) & 0xFFF);

  /* Convert 3 digit hexadecimal partnumber to ASCII and update TYPE4_STRINGS */
  AsciiSPrint (PartNumber, sizeof (PartNumber), "0x%03x", PartNum);
  CopyMem (&mArmMorelloSmbiosType4.Strings[0], PartNumber, sizeof (PartNumber));
}

/**
  Return the Processor Version string

  Fetch the SoC Revision from the Processor ID and
  form the processor version string.

  @param[out] ProcessorVerStr   Processor Version string.
  @param[in out] Length         Length of the processor version string.

  @retval EFI_SUCCESS           The string was returned successfully.
  @retval EFI_BUFFER_TOO_SMALL  Length of processor version string is not sufficient.
**/
EFI_STATUS
GetProcessorVersionStr (
  OUT CHAR8      *ProcessorVerStr,
  IN OUT UINT32  *Length
  )
{
  UINT32  MaxStrLen;
  UINT64  ProcessorId;
  UINT32  SiliconRevision;

  MaxStrLen = *Length;

  ProcessorId     = SmbiosGetProcessorId ();
  SiliconRevision = (ProcessorId >> 32);

  *Length = AsciiSPrint (
              ProcessorVerStr,
              MaxStrLen,
              "Morello-R%dP%d",
              (SiliconRevision & MORELLO_SILICON_REVISION_R_MASK) >> MORELLO_SILICON_REVISION_R_POS,
              (SiliconRevision & MORELLO_SILICON_REVISION_P_MASK) >> MORELLO_SILICON_REVISION_P_POS
              );

  if (*Length >= MaxStrLen) {
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}

/**
  Install SMBIOS Processor information Table

  Install the SMBIOS Processor information (type 4) table for
  Arm Morello System Development Platform.

  @param[in]  Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_NOT_FOUND         Unknown product id.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType4ProcessorInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  UINT64                    *ProcessorId = (UINT64 *)&(mArmMorelloSmbiosType4.Base.ProcessorId);
  UINT32                    StrLen;
  CHAR8                     *OptionalStrStart;
  ARM_MORELLO_SMBIOS_TYPE4  *SmbiosType4;
  UINT32                    Size;
  CHAR8                     ProcessorVerStr[MORELLO_PROCESSOR_VERSION_STR_MAX_LEN];

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType4)->Handle;

  mArmMorelloSmbiosType4.Base.SerialNumber = SerialNumberBase;

  /* Update processor-id and part number */
  *ProcessorId = ArmReadMidr ();
  UpdatePartNumber (*ProcessorId);

  ZeroMem (&ProcessorVerStr, sizeof (ProcessorVerStr));

  StrLen = MORELLO_PROCESSOR_VERSION_STR_MAX_LEN;
  Status = GetProcessorVersionStr (ProcessorVerStr, &StrLen);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to get the Processor Version String.\n"
      ));

    /* Install type 4 table */
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType4
                       );
    if (Status != EFI_SUCCESS) {
      DEBUG ((
        DEBUG_ERROR,
        "SMBIOS: Failed to install Type4 SMBIOS table.\n"
        ));
    }

    return Status;
  }

  Size        = sizeof (TYPE4_STRINGS) + sizeof (SMBIOS_TABLE_TYPE4) - 2;
  SmbiosType4 = AllocateZeroPool (Size + StrLen + 2);
  CopyMem (SmbiosType4, &mArmMorelloSmbiosType4, Size);

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)SmbiosType4)->Handle;

  SmbiosType4->Base.ProcessorVersion = ProcessorVersionBase;
  OptionalStrStart                   = SmbiosType4->Strings + sizeof (TYPE4_STRINGS) - 2;
  AsciiStrCpyS (OptionalStrStart, StrLen + 1, ProcessorVerStr);

  /* Install type 4 table */
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)SmbiosType4
                     );
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to install Type4 SMBIOS table.\n"
      ));
  }

  return Status;
}
