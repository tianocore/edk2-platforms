/** @file

  Copyright (C) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"
#include <Filecode.h>

#define FILECODE  LIBRARY_DXEAMLGENERATIONLIB_AMLOBJECTSDEBUG_FILECODE

/**
  DEBUG print a (VOID *)buffer in an array of HEX bytes

       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
  0000 54 48 49 53 20 49 53 20 41 20 53 41 4D 50 4C 45  THIS IS A SAMPLE
  0010 5F 42 55 46 46 45 52 01 02 5E 5C 30 31           _BUFFER..^\01

  @param[in]      Buffer      - Buffer containing buffer
  @param[in]      BufferSize  - Number of bytes to print

  @retval         EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
AmlDebugPrintBuffer (
  IN      VOID   *Buffer,
  IN      UINTN  BufferSize
  )
{
  UINTN  Column;
  UINTN  Index;
  UINTN  NumberOfColumns;
  UINT8  *Data;

  Data            = Buffer;
  NumberOfColumns = 16;
  // Header
  DEBUG ((DEBUG_VERBOSE, "      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"));
  for (Index = 0; Index < BufferSize;) {
    // Row Counter
    DEBUG ((DEBUG_VERBOSE, "%4X ", Index));

    // Hex ouput
    for (Column = 0; Column < NumberOfColumns; Column++) {
      if (Index + Column < BufferSize) {
        DEBUG ((DEBUG_VERBOSE, " %02X", Data[Index + Column]));
      } else {
        DEBUG ((DEBUG_VERBOSE, "   "));
      }
    }

    DEBUG ((DEBUG_VERBOSE, "  "));
    // Ascii ouput
    for (Column = 0; Column < NumberOfColumns; Column++) {
      if (Index + Column < BufferSize) {
        // Only print ACPI acceptable characters
        if (((Data[Index + Column] >= 0x30) &&      // '0' - '9'
             (Data[Index + Column] <= 0x39)) ||
            ((Data[Index + Column] >= 0x41) &&      // 'A' - 'Z'
             (Data[Index + Column] <= 0x5A)) ||
            (Data[Index + Column] == 0x5C) ||     // '\'
            (Data[Index + Column] == 0x5F) ||     // '_'
            (Data[Index + Column] == 0x5E)        // '^'
            )
        {
          DEBUG ((DEBUG_VERBOSE, "%c", Data[Index + Column]));
        } else {
          DEBUG ((DEBUG_VERBOSE, "."));
        }
      }
    }

    Index += NumberOfColumns;
    DEBUG ((DEBUG_VERBOSE, "\n"));
  }

  return EFI_SUCCESS;
}

/**
  DEBUG print an AML Object including an array of HEX bytes for the data

       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
  0000 54 48 49 53 20 49 53 20 41 20 53 41 4D 50 4C 45  THIS IS A SAMPLE
  0010 5F 42 55 46 46 45 52 01 02 5E 5C 30 31           _BUFFER..^\01
  Completed=(TRUE|FALSE)

  @param[in]      Object - AML_OBJECT_INSTANCE

  @retval         EFI_SUCCESS, EFI_INVALID_PARAMETER
**/
EFI_STATUS
EFIAPI
AmlDebugPrintObject (
  IN      AML_OBJECT_INSTANCE  *Object
  )
{
  if ((Object == NULL) || (Object->Signature != AML_OBJECT_INSTANCE_SIGNATURE)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "Object=0x%X, Size=0x%d\n",
    (UINTN)Object,
    Object->DataSize
    ));
  AmlDebugPrintBuffer (Object->Data, Object->DataSize);
  DEBUG ((DEBUG_VERBOSE, "Completed=%a\n", Object->Completed ? "TRUE" : "FALSE"));
  DEBUG ((DEBUG_VERBOSE, "\n"));
  return EFI_SUCCESS;
}

/**
  DEBUG print a linked list of AML buffer Objects in an array of HEX bytes

  @param[in]      ListHead - Head of AML_OBJECT_INSTANCE Linked List
**/
EFI_STATUS
EFIAPI
AmlDebugPrintLinkedObjects (
  IN      LIST_ENTRY  *ListHead
  )
{
  LIST_ENTRY           *Node;
  AML_OBJECT_INSTANCE  *Object;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "Printing AML_OBJECT_INSTANCE Linked List\n"));
  Node = GetNextNode (ListHead, ListHead);
  while (Node != ListHead) {
    Object = AML_OBJECT_INSTANCE_FROM_LINK (Node);
    AmlDebugPrintObject (Object);
    Node = GetNextNode (ListHead, Node);
  }

  return EFI_SUCCESS;
}
