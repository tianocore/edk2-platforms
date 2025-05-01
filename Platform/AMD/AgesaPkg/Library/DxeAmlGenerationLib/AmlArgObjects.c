/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"

/*
  Fill the DataBuffer with correct Arg Opcode based on provided argument number
  Valid Argument numbers are 0, 1, 2, 3, 4, 5 and 6.
  AML supports max 7 argument, i.e., Arg1, Arg2 ... Arg6.

  @param[in]    ArgN            - Argument Number
  @param[out]   ReturnData      - Allocated DataBuffer with encoded integer
  @param[out]   ReturnDataSize  - Size of ReturnData

  @return       EFI_SUCCESS     - Successful completion
  @return       EFI_OUT_OF_RESOURCES  - Failed to allocate ReturnDataBuffer
  @return       EFI_INVALID_PARAMETER - Invalid ArgN provided.
*/
EFI_STATUS
EFIAPI
InternalAmlArgBuffer (
  IN  OUT UINT8  ArgN,
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
      __FUNCTION__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  DataSize = 1;
  switch (ArgN) {
    case 0:
      Data[0] = AML_ARG0;
      break;
    case 1:
      Data[0] = AML_ARG1;
      break;
    case 2:
      Data[0] = AML_ARG2;
      break;
    case 3:
      Data[0] = AML_ARG3;
      break;
    case 4:
      Data[0] = AML_ARG4;
      break;
    case 5:
      Data[0] = AML_ARG5;
      break;
    case 6:
      Data[0] = AML_ARG6;
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
  Creates an ArgN Opcode object

  Arg Objects Encoding
    ArgObj := Arg0Op | Arg1Op | Arg2Op | Arg3Op | Arg4Op | Arg5Op | Arg6Op
    Arg0Op := 0x68
    Arg1Op := 0x69
    Arg2Op := 0x6A
    Arg3Op := 0x6B
    Arg4Op := 0x6C
    Arg5Op := 0x6D
    Arg6Op := 0x6E

  @param[in]      ArgN      - Argument Number to be encoded
  @param[in,out]  ListHead  - Head of Linked List of all AML Objects

  @return   EFI_SUCCESS     - Success
  @return   all others      - Fail
**/
EFI_STATUS
EFIAPI
AmlOpArgN (
  IN      UINT8       ArgN,
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
      __FUNCTION__,
      "ARGN_OPCODE"
      ));
    goto Done;
  }

  Status = InternalAmlArgBuffer (
             ArgN,
             (VOID **)&(Object->Data),
             &(Object->DataSize)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ERROR: ACPI Argument 0x%X object\n",
      __FUNCTION__,
      ArgN
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
