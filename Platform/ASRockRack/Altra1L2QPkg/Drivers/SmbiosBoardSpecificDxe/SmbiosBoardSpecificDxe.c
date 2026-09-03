/** @file
  This driver parses the mSmbiosBoardSpecificDxeDataTable structure
  and reports any generated data using SMBIOS protocol.

  Based on files under Nt32Pkg/MiscSubClassBoardSpecificDxe/

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "SmbiosBoardSpecificDxe.h"

#define SIZE_OF_HII_DATABASE_DEFAULT_STRINGS \
ADDITIONAL_STR_INDEX_MAX * SMBIOS_UNICODE_STRING_MAX_LENGTH

STATIC EFI_HANDLE          mSmbiosBoardSpecificDxeImageHandle;
STATIC EFI_STRING          mDefaultHiiDatabaseStr;
STATIC EFI_SMBIOS_PROTOCOL *mBoardSpecificDxeSmbios = NULL;

EFI_HII_HANDLE      mSmbiosBoardSpecificDxeHiiHandle;

/**
  Standard EFI driver point. This driver parses the mSmbiosBoardSpecificDataTable
  structure and reports any generated data using SMBIOS protocol.

  @param  ImageHandle          Handle for the image of this driver.
  @param  SystemTable          Pointer to the EFI System Table.

  @retval EFI_SUCCESS          The data was successfully stored.
          EFI_OUT_OF_RESOURCES There is no remaining memory to satisfy the request.
**/
EFI_STATUS
EFIAPI
SmbiosBoardSpecificDxeEntry (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  UINTN      Index;
  EFI_STATUS Status;

  mSmbiosBoardSpecificDxeImageHandle = ImageHandle;

  //
  // Allocate buffer to save default strings of HII Database
  //
  mDefaultHiiDatabaseStr = AllocateZeroPool (SIZE_OF_HII_DATABASE_DEFAULT_STRINGS);
  if (mDefaultHiiDatabaseStr == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] HII Database String allocates memory resource failed.\n",
      __FUNCTION__,
      __LINE__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Locate SMBIOS protocol and get HII Database Handle
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&mBoardSpecificDxeSmbios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Could not locate SMBIOS protocol. %r\n",
      __FUNCTION__,
      __LINE__,
      Status
      ));
    return Status;
  }

  mSmbiosBoardSpecificDxeHiiHandle = HiiAddPackages (
                                  &gEfiCallerIdGuid,
                                  mSmbiosBoardSpecificDxeImageHandle,
                                  SmbiosBoardSpecificDxeStrings,
                                  NULL
                                  );
  if (mSmbiosBoardSpecificDxeHiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Iterate through all Data Tables of each Type and call
  // function pointer to create and add Table accordingly
  //
  for (Index = 0; Index < mSmbiosBoardSpecificDxeDataTableEntries; Index++) {
    Status = (*mSmbiosBoardSpecificDxeDataTable[Index].Function)(
                mSmbiosBoardSpecificDxeDataTable[Index].RecordData,
                mSmbiosBoardSpecificDxeDataTable[Index].StrToken
                );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a]:[%dL] Could not install SMBIOS Table Type%d. %r\n",
        __FUNCTION__,
        __LINE__,
        ((EFI_SMBIOS_TABLE_HEADER *)(mSmbiosBoardSpecificDxeDataTable[Index].RecordData))->Type,
        Status
        ));
    }
  }

  //
  // Free buffer after all Tables were installed
  //
  FreePool (mDefaultHiiDatabaseStr);

  return Status;
}

/**
  Adds an SMBIOS record.

  @param  Buffer                 The data for the SMBIOS record.
                                 The format of the record is determined by
                                 EFI_SMBIOS_TABLE_HEADER.Type. The size of the
                                 formatted area is defined by EFI_SMBIOS_TABLE_HEADER.Length
                                 and either followed by a double-null (0x0000) or a set
                                 of null terminated strings and a null.
  @param  SmbiosHandle           A unique handle will be assigned to the SMBIOS record
                                 if not NULL.

  @retval EFI_SUCCESS            Record was added.
  @retval EFI_OUT_OF_RESOURCES   Record was not added due to lack of system resources.
  @retval EFI_ALREADY_STARTED    The SmbiosHandle passed in was already in use.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
**/
EFI_STATUS
SmbiosBoardSpecificDxeAddRecord (
  IN UINT8                 *Buffer,
  IN OUT EFI_SMBIOS_HANDLE *SmbiosHandle OPTIONAL
  )
{
  EFI_STATUS        Status;
  EFI_SMBIOS_HANDLE Handle;

  if (Buffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Buffer is NULL - Invalid parameter. %r\n",
      __FUNCTION__,
      __LINE__
      ));
    return EFI_INVALID_PARAMETER;
  }

  Handle = SMBIOS_HANDLE_PI_RESERVED;
  if (SmbiosHandle != NULL) {
    Handle = *SmbiosHandle;
  }

  Status = mBoardSpecificDxeSmbios->Add (
                                 mBoardSpecificDxeSmbios,
                                 NULL,
                                 &Handle,
                                 (EFI_SMBIOS_TABLE_HEADER *)Buffer
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] SMBIOS Type%d Table Log Failed! %r\n",
      __FUNCTION__,
      __LINE__,
      ((EFI_SMBIOS_TABLE_HEADER *)Buffer)->Type,
      Status
      ));
  }
  if (SmbiosHandle != NULL) {
    *SmbiosHandle = Handle;
  }

  return Status;
}

/**
  Fetches the number of handles of the specified SMBIOS type.

  @param  SmbiosType The type of SMBIOS record to look for.

  @retval UINTN      The number of handles.
**/
STATIC
UINTN
GetHandleCount (
  IN UINT8 SmbiosType
  )
{
  UINTN                   HandleCount;
  EFI_STATUS              Status;
  EFI_SMBIOS_HANDLE       SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER *Record;

  HandleCount = 0;
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  // Iterate through entries to get the number
  do {
    Status = mBoardSpecificDxeSmbios->GetNext (
                                   mBoardSpecificDxeSmbios,
                                   &SmbiosHandle,
                                   &SmbiosType,
                                   &Record,
                                   NULL
                                   );

    if (Status == EFI_SUCCESS) {
      HandleCount++;
    }
  } while (Status != EFI_NOT_FOUND);

  return HandleCount;
}

/**
  Fetches a list of the specified SMBIOS Table types.

  @param[in]   SmbiosType   The type of table to fetch.
  @param[out]  HandleArray  The array of handles.
  @param[out]  HandleCount  Number of handles in the array.
**/
VOID
SmbiosBoardSpecificDxeGetLinkTypeHandle (
  IN  UINT8         SmbiosType,
  OUT SMBIOS_HANDLE **HandleArray,
  OUT UINTN         *HandleCount
  )
{
  UINTN                   Index;
  EFI_STATUS              Status;
  EFI_SMBIOS_HANDLE       SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER *Record;

  if (SmbiosType > END_OF_SMBIOS_TABLE_TYPE) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Invalid SMBIOS Type.\n",
      __FUNCTION__,
      __LINE__
      ));
  }

  *HandleCount = GetHandleCount (SmbiosType);
  *HandleArray = AllocateZeroPool (sizeof (SMBIOS_HANDLE) * (*HandleCount));
  if (*HandleArray == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] HandleArray allocates memory resource failed.\n",
      __FUNCTION__,
      __LINE__
      ));
    *HandleCount = 0;
    return;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

  for (Index = 0; Index < (*HandleCount); Index++) {
    Status = mBoardSpecificDxeSmbios->GetNext (
                                   mBoardSpecificDxeSmbios,
                                   &SmbiosHandle,
                                   &SmbiosType,
                                   &Record,
                                   NULL
                                   );

    if (Status == EFI_SUCCESS) {
      (*HandleArray)[Index] = Record->Handle;
    } else {
      // It should never reach here
      ASSERT (FALSE);
      break;
    }
  }
}

/**
  Create SMBIOS Table Record with additional strings.

  @param[out]  TableRecord    Table Record is created.
  @param[in]   InputData      Input Table from Data Table.
  @param[in]   TableTypeSize  Size of Table with specified type.
  @param[in]   StrToken       Pointer to Token of additional strings in HII Database.
**/
VOID
SmbiosBoardSpecificDxeCreateTable (
  OUT VOID           **TableRecord,
  IN  VOID           **InputData,
  IN  UINT8          TableTypeSize,
  IN  STR_TOKEN_INFO *StrToken
  )
{
  CHAR8      *StrStart;
  UINT8      TableSize;
  UINT8      SmbiosAdditionalStrLen;
  UINT8      Index;
  EFI_STRING SmbiosAdditionalStr;

  if (*InputData == NULL ||
      StrToken == NULL ||
      TableTypeSize < sizeof (EFI_SMBIOS_TABLE_HEADER)
      )
  {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Invalid parameter to create SMBIOS Table\n",
      __FUNCTION__,
      __LINE__
      ));
    return;
  }

  //
  // Calculate size of Table.
  //
  if (StrToken->TokenLen != 0) {
    TableSize = TableTypeSize + 1; // Last byte is Null-terminated for Table with Null-terminated additional strings
    for (Index = 0; Index < StrToken->TokenLen; Index++) {
      SmbiosAdditionalStr = HiiGetPackageString (
                              &gEfiCallerIdGuid,
                              StrToken->TokenArray[Index],
                              NULL
                              );
      TableSize += StrLen (SmbiosAdditionalStr) + 1;
      FreePool (SmbiosAdditionalStr);
    }
  } else {
    TableSize = TableTypeSize + 1 + 1; // Double-null for Table with no additional strings
  }

  //
  // Allocate Table and copy strings from
  // HII Database for additional strings.
  //
  *TableRecord = AllocateZeroPool (TableSize);
  if (*TableRecord == NULL) {
    return;
  }
  CopyMem (*TableRecord, *InputData, TableTypeSize);
  StrStart = (CHAR8 *)(*TableRecord + TableTypeSize);
  for (Index = 0; Index < StrToken->TokenLen; Index++) {
    SmbiosAdditionalStr = HiiGetPackageString (
                            &gEfiCallerIdGuid,
                            StrToken->TokenArray[Index],
                            NULL
                            );
    SmbiosAdditionalStrLen = StrLen (SmbiosAdditionalStr) + 1;
    UnicodeStrToAsciiStrS (
      SmbiosAdditionalStr,
      StrStart,
      SmbiosAdditionalStrLen
    );
    FreePool (SmbiosAdditionalStr);
    StrStart += SmbiosAdditionalStrLen;
  }
}

/**
  Save default strings of HII Database in case multiple tables with the same type using
  these data for setting additional strings. After using, default strings will be set
  back again in HII Database by using SmbiosBoardSpecificDxeRestoreHiiDefaultString function
  for other tables with the same type to use. Before saving HII Database default strings,
  buffer for saving need to be available. Otherwise, that means a certain SMBIOS Table used
  this function but forget using SmbiosBoardSpecificDxeRestoreHiiDefaultString function to free
  buffer for other Tables to use so this check is for that purpose.

  @param[in]  StrToken     Pointer to Token of additional strings in HII Database.

  @retval     EFI_SUCCESS  Saved default strings of HII Database successfully.
              Other        Failed to save default strings of HII Database.
**/
EFI_STATUS
SmbiosBoardSpecificDxeSaveHiiDefaultString (
  IN STR_TOKEN_INFO *StrToken
  )
{
  UINT8      Index;
  UINT8      HiiDatabaseStrLen;
  EFI_STRING HiiDatabaseStr;

  if (StrToken == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Invalid String Tokens\n",
      __FUNCTION__,
      __LINE__
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Start saving HII Default Strings
  //
  for (Index = 0; Index < StrToken->TokenLen; Index++) {
    ASSERT (IsZeroBuffer ((VOID *)&mDefaultHiiDatabaseStr[Index * SMBIOS_STRING_MAX_LENGTH], SMBIOS_UNICODE_STRING_MAX_LENGTH - 1));
    HiiDatabaseStr = HiiGetPackageString (
                       &gEfiCallerIdGuid,
                       StrToken->TokenArray[Index],
                       NULL
                       );
    HiiDatabaseStrLen = (StrLen (HiiDatabaseStr) + 1) * sizeof (CHAR16);
    ASSERT (HiiDatabaseStrLen <= SMBIOS_UNICODE_STRING_MAX_LENGTH);
    UnicodeSPrint (
      (CHAR16 *)&mDefaultHiiDatabaseStr[Index * SMBIOS_STRING_MAX_LENGTH],
      HiiDatabaseStrLen,
      HiiDatabaseStr
      );
    FreePool (HiiDatabaseStr);
  }

  return EFI_SUCCESS;
}

/**
  Restore default strings of HII Database after using for setting additional strings.

  @param[in]  StrToken     Pointer to Token of additional strings in HII Database.

  @retval     EFI_SUCCESS  Restore default strings off HII Database successfully.
              Other        Failed to restore default strings of HII Database.
**/
EFI_STATUS
SmbiosBoardSpecificDxeRestoreHiiDefaultString (
  IN STR_TOKEN_INFO *StrToken
  )
{
  UINT8      Index;
  EFI_STATUS Status;

  if (StrToken == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Invalid String Tokens\n",
      __FUNCTION__,
      __LINE__
      ));
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < StrToken->TokenLen; Index++) {
    if (IsZeroBuffer ((VOID *)&mDefaultHiiDatabaseStr[Index * SMBIOS_STRING_MAX_LENGTH], SMBIOS_UNICODE_STRING_MAX_LENGTH - 1)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a]:[%dL] Default strings were not saved previously so failed to restore default strings.\n",
        __FUNCTION__,
        __LINE__
        ));
      return EFI_INVALID_PARAMETER;
    }

    Status = HiiSetString (
               mSmbiosBoardSpecificDxeHiiHandle,
               StrToken->TokenArray[Index],
               (EFI_STRING)&mDefaultHiiDatabaseStr[Index * SMBIOS_STRING_MAX_LENGTH],
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a]:[%dL] Failed to restore default strings\n",
        __FUNCTION__,
        __LINE__
        ));
      return Status;
    }
    ZeroMem ((VOID *)&mDefaultHiiDatabaseStr[Index * SMBIOS_STRING_MAX_LENGTH], SMBIOS_UNICODE_STRING_MAX_LENGTH - 1);
  }

  return Status;
}
