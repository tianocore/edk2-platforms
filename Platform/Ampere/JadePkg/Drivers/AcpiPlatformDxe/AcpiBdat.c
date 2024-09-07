/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"
#include "AcpiBdat.h"
#include <Guid/PlatformInfoHob.h>
#include <Library/HobLib.h>
#include <Library/AmpereCpuLib.h>

EFI_BDAT_ACPI_DESCRIPTION_TABLE  BdatTableHeaderTemplate = {
  __ACPI_HEADER (
    EFI_BDAT_TABLE_SIGNATURE,
    EFI_BDAT_ACPI_DESCRIPTION_TABLE,
    EFI_BDAT_TABLE_REVISION
    ),
  {
    .RegisterBitWidth = 8,
  }
};

#define NUM_SCHEMAS  1
#define SIZE_UEFI_SPD_SCHEMA(NumDimms) \
 ( \
  sizeof (BDAT_MEM_SPD_STRUCTURE) \
+ (sizeof (MEM_SPD_ENTRY_TYPE0) + MAX_SPD_BYTE) * (NumDimms) \
 )
#define BIOS_DATA_STRUCT_SIZE(NumDimms) \
 ( \
  sizeof (BDAT_STRUCTURE) \
+ sizeof (UINT32) * NUM_SCHEMAS \
+ SIZE_UEFI_SPD_SCHEMA (NumDimms) \
 )

BDAT_STRUCTURE  BdatStructTemplate = {
  {
    { 'B', 'D', 'A', 'T', 'H', 'E', 'A', 'D' },
    0,
    0,
    0,
    BDAT_PRIMARY_VER,
    BDAT_SECONDARY_VER,
    0,
    0,
    0,
  },{
    NUM_SCHEMAS,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  }
};

EFI_STATUS
CalculateCrc16 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT16  *CrcOut
  )
{
  UINT32  Crc;
  UINTN   Index;
  UINT8   *Byte;

  if ((Data == NULL) || (CrcOut == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Crc = 0x0000;
  for (Byte = (UINT8 *)Data; Byte < (UINT8 *)Data + DataSize; Byte++) {
    Crc ^= (UINT16)*Byte << 8;
    for (Index = 0; Index < 8; Index++) {
      Crc <<= 1;
      if (Crc & BIT16) {
        Crc ^= 0x1021;
      }
    }
  }

  *CrcOut = (UINT16)(Crc & 0xFFFF);
  return EFI_SUCCESS;
}

EFI_STATUS
AddUefiSpdSchema (
  OUT BDAT_MEM_SPD_STRUCTURE  *BdatMemSpdStructure,
  IN  PLATFORM_INFO_HOB       *PlatformHob,
  IN  UINT16                  NumDimms
  )
{
  EFI_GUID             BdatSpdSchemaGuid = BDAT_MEM_SPD_GUID;
  EFI_GUID             MemSpdDataIdGuid  = MEM_SPD_DATA_ID_GUID;
  MEM_SPD_ENTRY_TYPE0  *DimmSpdPointer;
  UINTN                DimmSlot;
  UINTN                Node;
  PLATFORM_DIMM        *DimmInfo;

  ZeroMem ((UINT8 *)BdatMemSpdStructure, sizeof (BDAT_MEM_SPD_STRUCTURE));

  BdatMemSpdStructure->SchemaHeader.SchemaId = BdatSpdSchemaGuid;
  BdatMemSpdStructure->SchemaHeader.DataSize = SIZE_UEFI_SPD_SCHEMA (NumDimms);

  BdatMemSpdStructure->SpdData.Header.MemSpdGuid = MemSpdDataIdGuid;
  BdatMemSpdStructure->SpdData.Header.Size       = SIZE_UEFI_SPD_SCHEMA (NumDimms) - sizeof (BDAT_SCHEMA_HEADER_STRUCTURE);
  BdatMemSpdStructure->SpdData.Header.Reserved   = 0;

  DimmSpdPointer = (MEM_SPD_ENTRY_TYPE0 *)((UINT8 *)BdatMemSpdStructure + sizeof (BDAT_MEM_SPD_STRUCTURE));
  for (DimmSlot = 0; DimmSlot < PLATFORM_DIMM_INFO_MAX_SLOT; DimmSlot += 2) {
    for (Node = 0; Node < GetNumberOfSupportedSockets (); Node++) {
      DimmInfo = &PlatformHob->DimmList.Dimm[DimmSlot];

      if (DimmInfo->Info.DimmStatus == DIMM_NOT_INSTALLED) {
        continue;
      }

      if (DimmInfo->NodeId != Node) {
        continue;
      }

      DimmSpdPointer->NumberOfBytes = MAX_SPD_BYTE;
      DimmSpdPointer->Header.Type   = MemSpdDataType0;
      DimmSpdPointer->Header.Size   = sizeof (MEM_SPD_ENTRY_TYPE0) + MAX_SPD_BYTE;

      DimmSpdPointer->MemoryLocation.Socket  = Node;
      DimmSpdPointer->MemoryLocation.Channel = (DimmSlot  - ((PLATFORM_DIMM_INFO_MAX_SLOT / 2) * Node)) / 2;
      DimmSpdPointer->MemoryLocation.Dimm    = 0;
      CopyMem (
        (VOID *)((UINT8 *)DimmSpdPointer + sizeof (MEM_SPD_ENTRY_TYPE0)),
        (VOID *)DimmInfo->SpdData.Data,
        MAX_SPD_BYTE
        );
      DimmSpdPointer = (MEM_SPD_ENTRY_TYPE0 *)((UINT8 *)DimmSpdPointer + DimmSpdPointer->Header.Size);
    }
  }

  // Schema CRC
  BdatMemSpdStructure->SpdData.Header.Crc = 0;
  gBS->CalculateCrc32 (
         &BdatMemSpdStructure->SpdData,
         BdatMemSpdStructure->SpdData.Header.Size,
         &BdatMemSpdStructure->SpdData.Header.Crc
         );

  BdatMemSpdStructure->SchemaHeader.Crc16 = 0;
  CalculateCrc16 (
    BdatMemSpdStructure,
    BdatMemSpdStructure->SchemaHeader.DataSize,
    &BdatMemSpdStructure->SchemaHeader.Crc16
    );

  return EFI_SUCCESS;
}

EFI_STATUS
PopulateBdatTable (
  OUT BDAT_STRUCTURE     *BdatStructure,
  IN  PLATFORM_INFO_HOB  *PlatformHob,
  IN  UINT16             NumDimms
  )
{
  EFI_STATUS              Status;
  BDAT_MEM_SPD_STRUCTURE  *BdatMemSpdStructure;

  ASSERT (BdatStructure != NULL);
  ASSERT (PlatformHob != NULL);

  // Add UEFI SPD Schema to BDAT Table
  BdatStructure->BdatSchemas.SchemaOffsets[0] = sizeof (BDAT_STRUCTURE) + sizeof (UINT32) * NUM_SCHEMAS;
  BdatMemSpdStructure                         = (BDAT_MEM_SPD_STRUCTURE *)((UINT8 *)BdatStructure + BdatStructure->BdatSchemas.SchemaOffsets[0]);
  Status                                      = AddUefiSpdSchema (BdatMemSpdStructure, PlatformHob, NumDimms);

  return Status;
}

EFI_STATUS
AcpiInstallBdatTable (
  VOID
  )
{
  PLATFORM_INFO_HOB                *PlatformHob;
  EFI_ACPI_TABLE_PROTOCOL          *AcpiTableProtocol;
  EFI_STATUS                       Status;
  EFI_TIME                         EfiTime;
  BDAT_STRUCTURE                   *BdatStructure;
  EFI_BDAT_ACPI_DESCRIPTION_TABLE  *BdatTablePointer;
  UINTN                            BdatTableKey;
  UINT16                           NumDimms;
  VOID                             *Hob;
  UINTN                            DimmSlot;
  PLATFORM_DIMM                    *DimmInfo;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the Platform HOB
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  // Figure out number of DIMMs
  NumDimms = 0;
  for (DimmSlot = 0; DimmSlot < PLATFORM_DIMM_INFO_MAX_SLOT; DimmSlot++) {
    DimmInfo = &PlatformHob->DimmList.Dimm[DimmSlot];
    if (DimmInfo->Info.DimmStatus != DIMM_NOT_INSTALLED) {
      NumDimms++;
    }
  }

  if (NumDimms == 0) {
    return EFI_NOT_FOUND;
  }

  BdatTablePointer = (EFI_BDAT_ACPI_DESCRIPTION_TABLE *)AllocateZeroPool (sizeof (EFI_BDAT_ACPI_DESCRIPTION_TABLE));
  if (BdatTablePointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize ACPI header, still lacks address and length
  CopyMem (
    (VOID *)BdatTablePointer,
    (VOID *)&BdatTableHeaderTemplate,
    sizeof (EFI_BDAT_ACPI_DESCRIPTION_TABLE)
    );

  Status = gBS->AllocatePool (
                  EfiACPIMemoryNVS,
                  BIOS_DATA_STRUCT_SIZE (NumDimms),
                  (VOID **)&BdatStructure
                  );
  if (EFI_ERROR (Status)) {
    FreePool (BdatTablePointer);
    return Status;
  }

  // Pointer into actual table
  CopyMem (
    (VOID *)BdatStructure,
    (VOID *)&BdatStructTemplate,
    sizeof (BdatStructTemplate)
    );
  BdatStructure->BdatHeader.BiosDataStructSize = BIOS_DATA_STRUCT_SIZE (NumDimms);

  Status = gRT->GetTime (&EfiTime, NULL);
  if (!EFI_ERROR (Status)) {
    BdatStructure->BdatSchemas.SchemaListLength = NUM_SCHEMAS;
    BdatStructure->BdatSchemas.Year             = EfiTime.Year;
    BdatStructure->BdatSchemas.Month            = EfiTime.Month;
    BdatStructure->BdatSchemas.Day              = EfiTime.Day;
    BdatStructure->BdatSchemas.Hour             = EfiTime.Hour;
    BdatStructure->BdatSchemas.Minute           = EfiTime.Minute;
    BdatStructure->BdatSchemas.Second           = EfiTime.Second;
  }

  Status = PopulateBdatTable (BdatStructure, PlatformHob, NumDimms);
  if (EFI_ERROR (Status)) {
    FreePool (BdatTablePointer);
    return Status;
  }

  // BDAT structure CRC
  BdatStructure->BdatHeader.Crc16 = 0;
  CalculateCrc16 (
    BdatStructure,
    BdatStructure->BdatHeader.BiosDataStructSize,
    &BdatStructure->BdatHeader.Crc16
    );

  // Setup Pointer in ACPI header
  BdatTablePointer->BdatGas.Address = (UINT64)BdatStructure;

  AcpiUpdateChecksum ((UINT8 *)BdatTablePointer, BdatTablePointer->Header.Length);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)BdatTablePointer,
                                BdatTablePointer->Header.Length,
                                &BdatTableKey
                                );
  FreePool (BdatTablePointer);

  return Status;
}
