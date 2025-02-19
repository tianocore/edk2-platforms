/** @file
  SMBIOS Type 4 (Processor information) table for ARM Morello System
  Development Platform.

  This file installs SMBIOS Type 4 (Processor information) Table for ARM
  Morello System Development Platform. It includes information about
  manufacture, family, processor id, maximum operating frequency, and
  other information related to the processor.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.5
**/

#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

#define TYPE4_STRINGS                                   \
  "0x000\0"                     /* Part Number */       \
  "ARM LTD\0"                   /* manufacturer */      \
  "Ironwood Socket\0"           /* socket type */       \
  "Morello-R0P1\0"              /* Processor Version */ \
  "000-0\0"                     /* Serial number */     \
  "\0"

typedef enum {
  PartNumber = 1,
  ManufacturerName,
  SocketTypeBase,
  ProcessorVersionBase,
  SerialNumberBase
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
    ProcessorVersionBase,    // Processor version
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
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  UINT64             *ProcessorId = (UINT64 *)&(mArmMorelloSmbiosType4.Base.ProcessorId);

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType4)->Handle;

  mArmMorelloSmbiosType4.Base.SerialNumber = SerialNumberBase;

  /* Update processor-id and part number */
  *ProcessorId = ArmReadMidr ();
  UpdatePartNumber (*ProcessorId);

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
