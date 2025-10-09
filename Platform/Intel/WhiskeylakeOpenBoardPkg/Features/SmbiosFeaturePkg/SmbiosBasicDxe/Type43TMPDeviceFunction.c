/** @file
  Smbios type 43.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  TPM Device (Type 43).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
TMPDeviceFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
	EFI_STATUS                  Status;
	CHAR8			   			*DescriptionStr;
	UINTN			    		DescriptionStrLen;
	SMBIOS_TABLE_TYPE43         *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE43         *SmbiosRecord;
   	EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;


	PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType43TMPDevice);

	
	//
    // Get board TPM Device description
    //
	
	DescriptionStr = PcdGetPtr (PcdSmbiosType43StringDescription);
    DescriptionStrLen = AsciiStrLen (DescriptionStr);
    ASSERT (DescriptionStrLen <= SMBIOS_STRING_MAX_LENGTH);

	//
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE43) + DescriptionStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
        ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
        return EFI_OUT_OF_RESOURCES;
	}

	CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE43));

	//
    // Fill in type 43 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_TPM_DEVICE;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE43);
    SmbiosRecord->Hdr.Handle = 0;

	//
    // Add strings to bottom of data block
   	//
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, DescriptionStr, DescriptionStrLen);

	//
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status;
}





