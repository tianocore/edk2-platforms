/** @file

  Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"
#include <Filecode.h>

#define FILECODE  LIBRARY_DXEAMLGENERATIONLIB_AMLLOCALOBJECTS_FILECODE

/**
  Fill the DataBuffer with correct Local Opcode based on provided argument number
  Valid Argument numbers are 0, 1, 2, 3, 4, 5 and 6.
  AML supports max 7 Local variables, i.e., Local1, Local2 ... Local6.

  @param[in]    LocalN          - Local variable Number
  @param[out]   ReturnData      - Allocated DataBuffer with encoded integer
  @param[out]   ReturnDataSize  - Size of ReturnData

  @return       EFI_SUCCESS     - Successful completion
  @return       EFI_OUT_OF_RESOURCES  - Failed to allocate ReturnDataBuffer
  @return       EFI_INVALID_PARAMETER - Invalid LocalN provided.
**/
EFI_STATUS
EFIAPI
InternalAmlLocalBuffer (
  IN  OUT UINT8  LocalN,
  OUT VOID       **ReturnData,
  OUT UINTN      *ReturnDataSize
  )
{
  UINT8  *Data;
  UINTN  DataSize;

  Data = AllocateZeroPool (sizeof (UINT8));
  if (Data == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ERROR: Failed to create Data Buffer.\n",
      __func__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  DataSize = 1;
  switch (LocalN) {
    case 0:
      Data[0] = AML_LOCAL0;
      break;
    case 1:
      Data[0] = AML_LOCAL1;
      break;
    case 2:
      Data[0] = AML_LOCAL2;
      break;
    case 3:
      Data[0] = AML_LOCAL3;
      break;
    case 4:
      Data[0] = AML_LOCAL4;
      break;
    case 5:
      Data[0] = AML_LOCAL5;
      break;
    case 6:
      Data[0] = AML_LOCAL6;
      break;
    case 7:
      Data[0] = AML_LOCAL7;
      break;
    default:
      FreePool (Data);
      return EFI_INVALID_PARAMETER;
  }

  *ReturnData     = (VOID *)Data;
  *ReturnDataSize = DataSize;

  return EFI_SUCCESS;
}

/**
  Creates an LocalN Opcode object

  Local Objects Encoding
    LocalObj := Local0Op | Local1Op | Local2Op | Local3Op | Local4Op | Local5Op | Local6Op | Local7Op
    Local0Op := 0x60
    Local1Op := 0x61
    Local2Op := 0x62
    Local3Op := 0x63
    Local4Op := 0x64
    Local5Op := 0x65
    Local6Op := 0x66
    Local7Op := 0x67

  @param[in]      LocalN      - Argument Number to be encoded
  @param[in,out]  ListHead  - Head of Linked List of all AML Objects

  @return   EFI_SUCCESS     - Success
  @return   all others      - Fail
**/
EFI_STATUS
EFIAPI
AmlOPLocalN (
  IN      UINT8       LocalN,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_DEVICE_ERROR;
  Object = NULL;

  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ERROR: Start %a object\n",
      __func__,
      "LOCALN_OPCODE"
      ));
    goto Done;
  }

  Status = InternalAmlLocalBuffer (
             LocalN,
             (VOID **)&(Object->Data),
             &(Object->DataSize)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ERROR: ACPI Argument 0x%X object\n",
      __func__,
      LocalN
      ));
    goto Done;
  }

  Object->Completed = TRUE;

  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}
