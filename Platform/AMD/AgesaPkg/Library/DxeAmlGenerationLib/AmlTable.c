/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"

// String Length Constants
#define OEM_ID_LENGTH        6
#define OEM_TABLE_ID_LENGTH  8
#define SIGNATURE_LENGTH     4
#define CREATOR_ID_LENGTH    4

/**
  Creates an AML Encoded Table
  Object must be created between AmlStart and AmlClose Phase

  DefBlockHeader  := TableSignature TableLength SpecCompliance CheckSum OemID
                     OemTableID OemRevision CreatorID CreatorRevision

  TableSignature  := DWordData      // As defined in section 5.2.3.
  TableLength     := DWordData      // Length of the table in bytes including the
                                    // block header
  SpecCompliance  := ByteData       // The revision of the structure.
  CheckSum        := ByteData       // Byte checksum of the entire table.
  OemID           := ByteData(6)    // OEM ID of up to 6 characters.
                                    // If the OEM ID is shorter than 6
                                    // characters, it can be terminated with a
                                    // NULL character.
  OemTableID      := ByteData(8)    // OEM Table ID of up to 8 characters.
                                    // If the OEM Table ID is shorter than
                                    // 8 characters, it can be terminated with
                                    // a NULL character.
  OemRevision     := DWordData      // OEM Table Revision.
  CreatorID       := DWordData      // Vendor ID of the ASL compiler.
  CreatorRevision := DWordData      // Revision of the ASL compiler.

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in]      TableNameString - Table Name
  @param[in]      ComplianceRev   - Compliance Revision
  @param[in]      OemId           - OEM ID
  @param[in]      OemTableId      - OEM ID of table
  @param[in]      OemRevision     - OEM Revision number
  @param[in]      CreatorId       - Vendor ID of the ASL compiler
  @param[in]      CreatorRevision - Vendor Revision of the ASL compiler
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlDefinitionBlock (
  IN      AML_FUNCTION_PHASE  Phase,
  IN      CHAR8               *TableNameString,
  IN      UINT8               ComplianceRev,
  IN      CHAR8               *OemId,
  IN      CHAR8               *OemTableId,
  IN      UINT32              OemRevision,
  IN      CHAR8               *CreatorId,
  IN      UINT32              CreatorRevision,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((Phase >= AmlInvalid) ||
      (ListHead == NULL) ||
      (TableNameString == NULL) ||
      (OemId == NULL) ||
      (OemTableId == NULL) ||
      (CreatorId == NULL) ||
      (AsciiStrLen (TableNameString) != SIGNATURE_LENGTH) ||
      (AsciiStrLen (OemId) > OEM_ID_LENGTH) ||
      (AsciiStrLen (OemTableId) > OEM_TABLE_ID_LENGTH) ||
      (AsciiStrLen (CreatorId) != CREATOR_ID_LENGTH))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, TableNameString, ListHead);
      // TermList is too complicated and must be added outside
      break;

    case AmlClose:
      // TermList should be closed already
      Status = InternalAmlLocateObjectByIdentifier (&Object, TableNameString, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locate %a object\n", __FUNCTION__, TableNameString));
        goto Done;
      }

      // Get rid of original Identifier data
      InternalFreeAmlObjectData (Object);

      // Collect child data and delete children
      Status = InternalAmlCollapseAndReleaseChildren (
                 &ChildObject,
                 &ChildCount,
                 &Object->Link,
                 ListHead
                 );
      if (EFI_ERROR (Status) ||
          (ChildObject->Data == NULL) ||
          (ChildObject->DataSize == 0))
      {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a has no child data.\n", __FUNCTION__, TableNameString));
        goto Done;
      }

      Object->DataSize = ChildObject->DataSize + sizeof (EFI_ACPI_DESCRIPTION_HEADER);
      Object->Data     = AllocateZeroPool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, TableNameString));
        goto Done;
      }

      // Fill table header with data
      // Signature
      CopyMem (
        &Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Signature)],
        TableNameString,
        AsciiStrLen (TableNameString)
        );

      // Table Length
      CopyMem (
        &Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Length)],
        (UINT32 *)&Object->DataSize,
        sizeof (UINT32)
        );

      // ACPI Table Version
      Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Revision)] = ComplianceRev;

      // OEM ID
      CopyMem (
        &Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, OemId)],
        OemId,
        AsciiStrLen (OemId)
        );

      // OEM Table ID
      CopyMem (
        &Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, OemTableId)],
        OemTableId,
        AsciiStrLen (OemTableId)
        );

      // OEM Table Version
      CopyMem (
        &Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, OemRevision)],
        (UINT8 *)&OemRevision,
        sizeof (UINT32)
        );

      // Creator ID
      CopyMem (
        &Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, CreatorId)],
        CreatorId,
        AsciiStrLen (CreatorId)
        );

      // Creator Version
      CopyMem (
        &Object->Data[OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, CreatorRevision)],
        (UINT8 *)&CreatorRevision,
        sizeof (UINT32)
        );

      // Copy rest of data into Object
      CopyMem (
        &Object->Data[sizeof (EFI_ACPI_DESCRIPTION_HEADER)],
        ChildObject->Data,
        ChildObject->DataSize
        );

      // Checksum Set on Table Install
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;
      Status            = EFI_SUCCESS;
      break;

    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    InternalFreeAmlObject (&ChildObject, ListHead);
  }

  return Status;
}
