/** @file
  Header file for the SmbiosPlatformDxe Driver.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_PLATFORM_DXE_H_
#define SMBIOS_PLATFORM_DXE_H_

#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>

#define NULL_TERMINATED_TYPE   0xFF
#define NULL_TERMINATED_TOKEN  0xFFFF

#define END_OF_SMBIOS_TABLE_TYPE          127
#define SMBIOS_UNICODE_STRING_MAX_LENGTH  (SMBIOS_STRING_MAX_LENGTH * sizeof (CHAR16))

typedef enum {
  ADDITIONAL_STR_INDEX_1 = 1,
  ADDITIONAL_STR_INDEX_2,
  ADDITIONAL_STR_INDEX_3,
  ADDITIONAL_STR_INDEX_4,
  ADDITIONAL_STR_INDEX_5,
  ADDITIONAL_STR_INDEX_6,
  ADDITIONAL_STR_INDEX_7,
  ADDITIONAL_STR_INDEX_8,
  ADDITIONAL_STR_INDEX_9,
  ADDITIONAL_STR_INDEX_MAX
} ADDITIONAl_STR_INDEX;

//
// Data table entry update function.
//
typedef EFI_STATUS (EFIAPI SMBIOS_PLATFORM_DXE_DATA_FUNCTION)(
  IN VOID *RecordData,
  IN VOID *StrToken
  );

#pragma pack(1)
//
//  Data table entry definition.
//
typedef struct {
  //
  // Intermediate input data for SMBIOS record
  //
  VOID                                 *RecordData;
  VOID                                 *StrToken;
  SMBIOS_PLATFORM_DXE_DATA_FUNCTION    *Function;
} SMBIOS_PLATFORM_DXE_DATA_TABLE;

typedef struct {
  UINT16    TokenArray[ADDITIONAL_STR_INDEX_MAX];
  UINT8     TokenLen;
} STR_TOKEN_INFO;
#pragma pack()

//
// SMBIOS table extern definitions.
//
#define SMBIOS_PLATFORM_DXE_TABLE_EXTERNS(SMBIOS_TYPE, BASE_NAME) \
extern SMBIOS_TYPE BASE_NAME ## Data[]; \
extern STR_TOKEN_INFO BASE_NAME ## StrToken[]; \
extern SMBIOS_PLATFORM_DXE_DATA_FUNCTION BASE_NAME ## Function;

//
// SMBIOS data table entries.
//
// This is used to define tables for structure pointer, functions and
// string Tokens in order to iterate through the list of tables, populate
// them and add them into the system.
#define SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION(BASE_NAME) \
{ \
  BASE_NAME ## Data, \
  BASE_NAME ## StrToken, \
  BASE_NAME ## Function \
}

//
// Global definition macros.
//
#define SMBIOS_PLATFORM_DXE_TABLE_DATA(SMBIOS_TYPE, BASE_NAME) \
  SMBIOS_TYPE BASE_NAME ## Data[]

#define SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA(BASE_NAME) \
  STR_TOKEN_INFO BASE_NAME ## StrToken[]

#define SMBIOS_PLATFORM_DXE_TABLE_FUNCTION(BASE_NAME) \
  EFI_STATUS EFIAPI BASE_NAME ## Function( \
  IN VOID *RecordData, \
  IN VOID *StrToken \
  )

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
SmbiosPlatformDxeAddRecord (
  IN     UINT8              *Buffer,
  IN OUT EFI_SMBIOS_HANDLE  *SmbiosHandle OPTIONAL
  );

/**
  Fetches a list of the specified SMBIOS Table types.

  @param[in]   SmbiosType   The type of table to fetch.
  @param[out]  HandleArray  The array of handles.
  @param[out]  HandleCount  Number of handles in the array.
**/
VOID
SmbiosPlatformDxeGetLinkTypeHandle (
  IN  UINT8          SmbiosType,
  OUT SMBIOS_HANDLE  **HandleArray,
  OUT UINTN          *HandleCount
  );

/**
  Create SMBIOS Table Record with additional strings.

  @param[out]  TableRecord    Table Record is created.
  @param[in]   InputData      Input Table from Data Table.
  @param[in]   TableTypeSize  Size of Table with specified type.
  @param[in]   StrToken       Pointer to Token of additional strings in HII Database.
**/
VOID
SmbiosPlatformDxeCreateTable (
  OUT VOID            **TableRecord,
  IN  VOID            **InputData,
  IN  UINT8           TableTypeSize,
  IN  STR_TOKEN_INFO  *StrToken
  );

/**
  Save default strings of HII Database in case multiple tables with the same type using
  these data for setting additional strings. After using, default strings will be set
  back again in HII Database for other tables with the same type to use.

  @param[in]  StrToken     Pointer to Token of additional strings in HII Database.

  @retval     EFI_SUCCESS  Saved default strings of HII Database successfully.
              Other        Failed to save default strings of HII Database.
**/
EFI_STATUS
SmbiosPlatformDxeSaveHiiDefaultString (
  IN STR_TOKEN_INFO  *StrToken
  );

/**
  Restore default strings of HII Database after using for setting additional strings.

  @param[in]  StrToken     Pointer to Token of additional strings in HII Database.

  @retval     EFI_SUCCESS  Restore default strings off HII Database successfully.
              Other        Failed to restore default strings of HII Database.
**/
EFI_STATUS
SmbiosPlatformDxeRestoreHiiDefaultString (
  IN STR_TOKEN_INFO  *StrToken
  );

//
// Data Table Array
//
extern SMBIOS_PLATFORM_DXE_DATA_TABLE  mSmbiosPlatformDxeDataTable[];

//
// Data Table Array Entries
//
extern UINTN  mSmbiosPlatformDxeDataTableEntries;

//
// HII Database Handle
//
extern EFI_HII_HANDLE  mSmbiosPlatformDxeHiiHandle;

extern UINT8  SmbiosPlatformDxeStrings[];

#endif // SMBIOS_PLATFORM_DXE_H_
