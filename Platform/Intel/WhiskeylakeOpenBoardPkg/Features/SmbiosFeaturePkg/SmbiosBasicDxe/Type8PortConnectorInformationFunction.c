/** @file
  Smbios type 8.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  Port Connector Information (Type 8).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
PortConnectorInformationFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *InternalReferenceDesignatorStr;
    CHAR8                       *ExternalReferenceDesignatorStr;
    UINTN                       InternalReferenceDesignatorStrLen;
    UINTN                       ExternalReferenceDesignatorStrLen;
    SMBIOS_TABLE_TYPE8          *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE8          *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;

    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType8PortConnectorInformation);

    //
    // Get board Internal reference dsignator string
    //
    InternalReferenceDesignatorStr = PcdGetPtr (PcdSmbiosType8StringInternalReferenceDesignator);
    InternalReferenceDesignatorStrLen = AsciiStrLen (InternalReferenceDesignatorStr);
    ASSERT (InternalReferenceDesignatorStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board ExternalReferenceDesignator string
    //
    ExternalReferenceDesignatorStr = PcdGetPtr (PcdSmbiosType8StringExternalReferenceDesignator);
    ExternalReferenceDesignatorStrLen = AsciiStrLen (ExternalReferenceDesignatorStr);
    ASSERT (ExternalReferenceDesignatorStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE8) + InternalReferenceDesignatorStrLen + 1 + ExternalReferenceDesignatorStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
        ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
        return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE8));

    //
    // Fill in type 8 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE8);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, InternalReferenceDesignatorStr, InternalReferenceDesignatorStrLen);
    StringOffset += InternalReferenceDesignatorStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ExternalReferenceDesignatorStr, ExternalReferenceDesignatorStrLen);

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status; 
}