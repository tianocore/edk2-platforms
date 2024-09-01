/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <Guid/SmBios.h>
#include <Library/AmpereCpuLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>

#define TYPE16_ADDITIONAL_STRINGS        \
  "\0" /* no string*/

#define TYPE17_ADDITIONAL_STRINGS        \
  "Device Locator not set \0"  \
  "Bank Locator not set \0"    \
  "Manufacturer not set \0"    \
  "Serial Number not set \0"   \
  "Asset Tag not set \0"       \
  "Part Number not set \0"     \

#define TYPE19_ADDITIONAL_STRINGS        \
  "\0" /* no string */

//
// Type definition and contents of the default SMBIOS table.
// This table covers only the minimum structures required by
// the SMBIOS specification (section 6.2, version 3.0)
//
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE16 Base;
  CHAR8               Strings[sizeof (TYPE16_ADDITIONAL_STRINGS)];
} ARM_TYPE16;

typedef struct {
  SMBIOS_TABLE_TYPE17 Base;
  CHAR8               Strings[sizeof (TYPE17_ADDITIONAL_STRINGS)];
} ARM_TYPE17;

typedef struct {
  SMBIOS_TABLE_TYPE19 Base;
  CHAR8               Strings[sizeof (TYPE19_ADDITIONAL_STRINGS)];
} ARM_TYPE19;

#pragma pack()
// Type 16 Physical Memory Array
STATIC ARM_TYPE16 mArmDefaultType16 = {
  {
    {                                        // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE16),          // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    MemoryArrayLocationSystemBoard,   // on motherboard
    MemoryArrayUseSystemMemory,       // system RAM
    MemoryErrorCorrectionMultiBitEcc, // ECC RAM
    0x80000000,
    0xFFFE,   // No error information structure
    0x10,
    0x40000000000ULL,
  },
  TYPE16_ADDITIONAL_STRINGS
};

// Type 17 Memory Device
STATIC ARM_TYPE17 mArmDefaultType17 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_MEMORY_DEVICE,
      sizeof (SMBIOS_TABLE_TYPE17),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    0xFFFF,                         // array to which this module belongs
    0xFFFE,                         // no errors
    72,                             // single DIMM with ECC
    64,                             // data width of this device (64-bits)
    0,                              // no module installed
    0x09,                           // DIMM
    1,                              // part of a set
    1,                              // device locator
    2,                              // bank locator
    MemoryTypeDdr4,                 // DDR4
    {},                             // type detail
    0,                              // ? MHz
    3,                              // manufacturer
    4,                              // serial
    5,                              // asset tag
    6,                              // part number
    0,                              // rank
  },
  TYPE17_ADDITIONAL_STRINGS
};

// Type 19 Memory Array Mapped Address
STATIC ARM_TYPE19 mArmDefaultType19 = {
  {
    {  // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS,
      sizeof (SMBIOS_TABLE_TYPE19),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    0xFFFFFFFF, // invalid, look at extended addr field
    0xFFFFFFFF,
    0xFFFF,    // handle
    1,
    0x0,
    0x0,
  },
  TYPE19_ADDITIONAL_STRINGS
};

typedef struct _JEDEC_MF_ID {
  UINT8 VendorId;
  CHAR8 *ManufacturerString;
} JEDEC_MF_ID;

JEDEC_MF_ID Bank0Table[] = {
  { 0x01, "AMD" },
  { 0x04, "Fujitsu" },
  { 0x07, "Hitachi" },
  { 0x89, "Intel" },
  { 0x10, "NEC" },
  { 0x97, "Texas Instrument" },
  { 0x98, "Toshiba" },
  { 0x1c, "Mitsubishi" },
  { 0x1f, "Atmel" },
  { 0x20, "STMicroelectronics" },
  { 0xa4, "IBM" },
  { 0x2c, "Micron Technology" },
  { 0xad, "SK Hynix" },
  { 0xb0, "Sharp" },
  { 0xb3, "IDT" },
  { 0x3e, "Oracle" },
  { 0xbf, "SST" },
  { 0x40, "ProMos/Mosel" },
  { 0xc1, "Infineon" },
  { 0xc2, "Macronix" },
  { 0x45, "SanDisk" },
  { 0xce, "Samsung" },
  { 0xda, "Winbond" },
  { 0xe0, "LG Semi" },
  { 0x62, "Sanyo" },
  { 0xff, "Undefined" }
};

JEDEC_MF_ID Bank1Table[] = {
  { 0x98, "Kingston" },
  { 0xba, "PNY" },
  { 0x4f, "Transcend" },
  { 0x7a, "Apacer" },
  { 0xff, "Undefined" }
};

JEDEC_MF_ID Bank2Table[] = {
  { 0x9e, "Corsair" },
  { 0xfe, "Elpida" },
  { 0xff, "Undefined" }
};

JEDEC_MF_ID Bank3Table[] = {
  { 0x0b, "Nanya" },
  { 0x94, "Mushkin" },
  { 0x25, "Kingmax" },
  { 0xff, "Undefined" }
};

JEDEC_MF_ID Bank4Table[] = {
  { 0xb0, "OCZ" },
  { 0xcb, "A-DATA" },
  { 0xcd, "G Skill" },
  { 0xef, "Team" },
  { 0xff, "Undefined" }
};

JEDEC_MF_ID Bank5Table[] = {
  { 0x02, "Patriot" },
  { 0x9b, "Crucial" },
  { 0x51, "Qimonda" },
  { 0x57, "AENEON" },
  { 0xf7, "Avant" },
  { 0xff, "Undefined" }
};

JEDEC_MF_ID Bank6Table[] = {
  { 0x34, "Super Talent" },
  { 0xd3, "Silicon Power" },
  { 0xff, "Undefined" }
};

JEDEC_MF_ID Bank7Table[] = {
  { 0xff, "Undefined" }
};

JEDEC_MF_ID *ManufacturerJedecIdBankTable[] = {
  Bank0Table,
  Bank1Table,
  Bank2Table,
  Bank3Table,
  Bank4Table,
  Bank5Table,
  Bank6Table,
  Bank7Table
};

STATIC
UINTN
GetStringPackSize (
  CHAR8 *StringPack
  )
{
  UINTN StrCount;
  CHAR8 *StrStart;

  if ((*StringPack == 0) && (*(StringPack + 1) == 0)) {
    return 0;
  }

  // String section ends in double-null (0000h)
  for (StrCount = 0, StrStart = StringPack;
       ((*StrStart != 0) || (*(StrStart + 1) != 0)); StrStart++, StrCount++)
  {
  }

  return StrCount + 2; // Included the double NULL
}

// Update String at String number to String Pack
EFI_STATUS
UpdateStringPack (
  CHAR8 *StringPack,
  CHAR8 *String,
  UINTN StringNumber
  )
{
  CHAR8 *StrStart;
  UINTN StrIndex;
  UINTN InputStrLen;
  UINTN TargetStrLen;
  UINTN BufferSize;
  CHAR8 *Buffer;

  StrStart = StringPack;
  for (StrIndex = 1; StrIndex < StringNumber; StrStart++) {
    // A string ends in 00h
    if (*StrStart == 0) {
      StrIndex++;
    }
    // String section ends in double-null (0000h)
    if ((*StrStart == 0) && (*(StrStart + 1) == 0)) {
      return EFI_NOT_FOUND;
    }
  }

  if (*StrStart == 0) {
    StrStart++;
  }

  InputStrLen = AsciiStrLen (String);
  TargetStrLen = AsciiStrLen (StrStart);
  BufferSize = GetStringPackSize (StrStart + TargetStrLen + 1);

  // Replace the String if length matched
  // OR this is the last string
  if ((InputStrLen == TargetStrLen) || (BufferSize == 0)) {
    CopyMem (StrStart, String, InputStrLen);
  }
  // Otherwise, buffer is needed to apply new string
  else {
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Buffer, StrStart + TargetStrLen + 1, BufferSize);
    CopyMem (StrStart, String, InputStrLen + 1);
    CopyMem (StrStart + InputStrLen + 1, Buffer, BufferSize);

    FreePool (Buffer);
  }

  return EFI_SUCCESS;
}

UINT8
GetMemoryType (
  IN UINT8 *SpdData
  )
{
  return (SpdData[2] == 0x08) ? 1 :    // DDR2
         (SpdData[2] == 0x0B) ? 2 :    // DDR3
         (SpdData[2] == 0x0C) ? 3 : 0; // DDR4
}

EFI_STATUS
UpdateManufacturer (
  IN VOID  *Table,
  IN UINT8 *SpdData
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       i;
  UINT8       Data8;
  UINT8       MemType;
  JEDEC_MF_ID *IdTblPtr = NULL;

  MemType = GetMemoryType (SpdData);

  switch (MemType) {
  case 1:
    for (i = 0; i < 8; i++) {      // DDR2
      Data8 = SpdData[64 + i];
      if (Data8 != 0x7f) {
        break;
      }
    }
    break;

  case 2:                    // DDR3
    i = SpdData[117] & 0x7f; // Remove parity bit
    Data8 = SpdData[118];
    break;

  case 3:                    // DDR4
    i = SpdData[320] & 0x7f; // Remove parity bit
    Data8 = SpdData[321];
    break;

  default:
    return EFI_UNSUPPORTED;  // Not supported
  }

  if (i > 7) {
    i = 7;
  }
  IdTblPtr = ManufacturerJedecIdBankTable[i];

  // Search in Manufacturer table
  while ((IdTblPtr->VendorId != Data8) && (IdTblPtr->VendorId != 0xff)) {
    IdTblPtr++;
  }

  if (IdTblPtr->VendorId != 0xff) {
    Status = UpdateStringPack (
               ((ARM_TYPE17 *)Table)->Strings,
               IdTblPtr->ManufacturerString,
               ((ARM_TYPE17 *)Table)->Base.Manufacturer
               );
  }

  return Status;
}

EFI_STATUS
UpdateSerialNumber (
  IN VOID  *Table,
  IN UINT8 *SpdData
  )
{
  UINT8 MemType;
  UINTN Offset;
  CHAR8 Str[64];

  MemType = GetMemoryType (SpdData);

  switch (MemType) {
  case 1:
    Offset = 95;
    break;

  case 2:                          // DDR3
    Offset = 122;
    break;

  case 3:                          // DDR4
    Offset = 325;
    break;

  default:
    return EFI_UNSUPPORTED;        // Not supported
  }

  AsciiSPrint (
    Str,
    sizeof (Str),
    "%02X%02X%02X%02X",
    SpdData[Offset],
    SpdData[Offset + 1],
    SpdData[Offset + 2],
    SpdData[Offset + 3]
    );

  return UpdateStringPack (
           ((ARM_TYPE17 *)Table)->Strings,
           Str,
           ((ARM_TYPE17 *)Table)->Base.SerialNumber
           );
}

CHAR8
Printable (
  IN CHAR8 Character
  )
{
  if((Character >= 0x20) && (Character <= 0x7E)) {
    return Character;
  }

  return ' ';
}

EFI_STATUS
UpdatePartNumber (
  IN VOID  *Table,
  IN UINT8 *SpdData
  )
{
  UINT8 MemType;
  UINTN Offset;
  CHAR8 Str[64];

  MemType = GetMemoryType (SpdData);

  switch (MemType) {
  case 1:
    Offset = 73;
    break;

  case 2:                          // DDR3
    Offset = 128;
    break;

  case 3:                          // DDR4
    Offset = 329;
    break;

  default:
    return EFI_UNSUPPORTED;        // Not supported
  }

  AsciiSPrint (
    Str,
    sizeof (Str),
    "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
    Printable (SpdData[Offset]),
    Printable (SpdData[Offset + 1]),
    Printable (SpdData[Offset + 2]),
    Printable (SpdData[Offset + 3]),
    Printable (SpdData[Offset + 4]),
    Printable (SpdData[Offset + 5]),
    Printable (SpdData[Offset + 6]),
    Printable (SpdData[Offset + 7]),
    Printable (SpdData[Offset + 8]),
    Printable (SpdData[Offset + 9]),
    Printable (SpdData[Offset + 10]),
    Printable (SpdData[Offset + 11]),
    Printable (SpdData[Offset + 12]),
    Printable (SpdData[Offset + 13]),
    Printable (SpdData[Offset + 14]),
    Printable (SpdData[Offset + 15]),
    Printable (SpdData[Offset + 16]),
    Printable (SpdData[Offset + 17])
    );

  return UpdateStringPack (
           ((ARM_TYPE17 *)Table)->Strings,
           Str,
           ((ARM_TYPE17 *)Table)->Base.PartNumber
           );
}

/**
   Install SMBIOS Type 16 17 and 19 table

   @param  Smbios               SMBIOS protocol.
**/
EFI_STATUS
InstallMemStructures (
  IN EFI_SMBIOS_PROTOCOL *Smbios
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  EFI_SMBIOS_HANDLE  Type16Handle;
  PLATFORM_INFO_HOB  *PlatformHob;
  PLATFORM_DIMM      *Dimm;
  CHAR8              *Table;
  VOID               *Hob;
  UINTN              Index;
  UINTN              SlotIndex;
  UINTN              MemRegionIndex;
  UINT64             MemorySize = 0;
  CHAR8              Str[64];

  ASSERT (Smbios != NULL);

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  Table = AllocateZeroPool (sizeof (ARM_TYPE17));
  if (Table == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for ( Index = 0; Index < GetNumberOfSupportedSockets (); Index++ ) {
    // Copy template to Type 16
    CopyMem (Table, (VOID *)&mArmDefaultType16, sizeof (ARM_TYPE16));

    ((ARM_TYPE16 *)Table)->Base.MaximumCapacity = 0x80000000;
    ((ARM_TYPE16 *)Table)->Base.ExtendedMaximumCapacity = 0x40000000000ULL; /* 4TB per socket */
    ((ARM_TYPE16 *)Table)->Base.MemoryErrorCorrection = MemoryErrorCorrectionMultiBitEcc;

    // Install Type 16 and hold the handle so that the subsequence type17/19 could use
    Type16Handle = ((ARM_TYPE16 *)Table)->Base.Hdr.Handle;
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &Type16Handle,
                       (EFI_SMBIOS_TABLE_HEADER *)Table
                       );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: adding SMBIOS type 16 socket %d failed\n", __func__, Index));
      FreePool (Table);
      // stop adding rather than continuing
      return Status;
    }

    for (SlotIndex = 0; SlotIndex < PlatformHob->DimmList.BoardDimmSlots; SlotIndex++) {
      // Copy Tempplate to Type 17
      CopyMem (Table, (VOID *)&mArmDefaultType17, sizeof (ARM_TYPE17));

      // Fill up type 17 table's content here
      Dimm = &PlatformHob->DimmList.Dimm[SlotIndex];
      if (Dimm->NodeId != Index) {
        continue;
      }

      if (Dimm->Info.DimmStatus == DIMM_INSTALLED_OPERATIONAL) {

        UpdateManufacturer ((VOID *)Table, Dimm->SpdData.Data);
        UpdateSerialNumber ((VOID *)Table, Dimm->SpdData.Data);
        UpdatePartNumber ((VOID *)Table, Dimm->SpdData.Data);

        MemorySize = Dimm->Info.DimmSize * 1024;
        if (MemorySize >= 0x7FFF) {
          ((ARM_TYPE17 *)Table)->Base.Size = 0x7FFF;
          ((ARM_TYPE17 *)Table)->Base.ExtendedSize = MemorySize;
        } else {
          ((ARM_TYPE17 *)Table)->Base.Size = (UINT16)MemorySize;
          ((ARM_TYPE17 *)Table)->Base.ExtendedSize = 0;
        }

        ((ARM_TYPE17 *)Table)->Base.MemoryType = 0x1A; /* DDR4 */
        ((ARM_TYPE17 *)Table)->Base.Speed = (UINT16)PlatformHob->DramInfo.MaxSpeed;
        ((ARM_TYPE17 *)Table)->Base.ConfiguredMemoryClockSpeed = (UINT16)PlatformHob->DramInfo.MaxSpeed;
        ((ARM_TYPE17 *)Table)->Base.Attributes = Dimm->Info.DimmNrRank & 0x0F;
        ((ARM_TYPE17 *)Table)->Base.ConfiguredVoltage = 1200;
        ((ARM_TYPE17 *)Table)->Base.MinimumVoltage = 1140;
        ((ARM_TYPE17 *)Table)->Base.MaximumVoltage = 1260;
        ((ARM_TYPE17 *)Table)->Base.DeviceSet = 0; // None

        if (Dimm->Info.DimmType == UDIMM || Dimm->Info.DimmType == SODIMM) {
          ((ARM_TYPE17 *)Table)->Base.TypeDetail.Unbuffered = 1; /* BIT 14: unregistered */
        } else if (Dimm->Info.DimmType == RDIMM
                   || Dimm->Info.DimmType == LRDIMM
                   || Dimm->Info.DimmType == RSODIMM)
        {
          ((ARM_TYPE17 *)Table)->Base.TypeDetail.Registered = 1; /* BIT 13: registered */
        }
        // We should determine if need to set technology to NVDIMM-* when supported
        ((ARM_TYPE17 *)Table)->Base.MemoryTechnology = 0x3; // DRAM
      }
      AsciiSPrint (Str, sizeof (Str), "Socket %d DIMM %d", Index, SlotIndex);
      UpdateStringPack (((ARM_TYPE17 *)Table)->Strings, Str, ((ARM_TYPE17 *)Table)->Base.DeviceLocator);
      AsciiSPrint (Str, sizeof (Str), "Bank %d", SlotIndex);
      UpdateStringPack (((ARM_TYPE17 *)Table)->Strings, Str, ((ARM_TYPE17 *)Table)->Base.BankLocator);
      AsciiSPrint (Str, sizeof (Str), "Array %d Asset Tag %d", Index, SlotIndex);
      UpdateStringPack (((ARM_TYPE17 *)Table)->Strings, Str, ((ARM_TYPE17 *)Table)->Base.AssetTag);

      // Update Type 16 handle
      ((ARM_TYPE17 *)Table)->Base.MemoryArrayHandle = Type16Handle;

      // Install structure
      SmbiosHandle = ((ARM_TYPE17 *)Table)->Base.Hdr.Handle;
      Status = Smbios->Add (
                         Smbios,
                         NULL,
                         &SmbiosHandle,
                         (EFI_SMBIOS_TABLE_HEADER *)Table
                         );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: adding SMBIOS type 17 Socket %d Slot %d failed\n",
          __func__,
          Index,
          SlotIndex
          ));
        FreePool (Table);
        // stop adding rather than continuing
        return Status;
      }
    }

    for (MemRegionIndex = 0; MemRegionIndex < PlatformHob->DramInfo.NumRegion; MemRegionIndex++) {
      // Copy Tempplate to Type 19
      CopyMem (Table, (VOID *)&mArmDefaultType19, sizeof (ARM_TYPE19));

      if (PlatformHob->DramInfo.NvdRegion[MemRegionIndex] > 0
          || PlatformHob->DramInfo.Socket[MemRegionIndex] != Index)
      {
        continue;
      }

      ((ARM_TYPE19 *)Table)->Base.StartingAddress = 0xFFFFFFFF;
      ((ARM_TYPE19 *)Table)->Base.EndingAddress = 0xFFFFFFFF;
      ((ARM_TYPE19 *)Table)->Base.ExtendedStartingAddress = PlatformHob->DramInfo.Base[MemRegionIndex];
      ((ARM_TYPE19 *)Table)->Base.ExtendedEndingAddress = PlatformHob->DramInfo.Base[MemRegionIndex] +
                                                          PlatformHob->DramInfo.Size[MemRegionIndex] - 1;

      if (MemorySize != 0) {
        ((ARM_TYPE19 *)Table)->Base.PartitionWidth = (PlatformHob->DramInfo.Size[MemRegionIndex] - 1) / MemorySize + 1;
      }

      // Update Type 16 handle
      ((ARM_TYPE19 *)Table)->Base.MemoryArrayHandle = Type16Handle;

      // Install structure
      SmbiosHandle = ((ARM_TYPE19 *)Table)->Base.Hdr.Handle;
      Status = Smbios->Add (
                         Smbios,
                         NULL,
                         &SmbiosHandle,
                         (EFI_SMBIOS_TABLE_HEADER *)Table
                         );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: adding SMBIOS type 19 Socket %d MemRegion %d failed\n",
          __func__,
          Index,
          MemRegionIndex
          ));
        FreePool (Table);
        // stop adding rather than continuing
        return Status;
      }
    }
  }

  FreePool (Table);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SmbiosMemInfoDxeEntry (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS          Status;
  EFI_SMBIOS_PROTOCOL *Smbios;

  //
  // Find the SMBIOS protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate SMBIOS Protocol"));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = InstallMemStructures (Smbios);
  DEBUG ((DEBUG_ERROR, "SmbiosMemInfoDxe install: %r\n", Status));

  return Status;
}
