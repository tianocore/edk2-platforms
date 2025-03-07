/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"

/**
  Free all the children AML_OBJECT_INSTANCE(s) of ListHead.
  Will not free ListHead nor an Object containing ListHead.

  @param[in,out]  ListHead  - Head of linked list of Objects

  @retval         EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
AmlFreeObjectList (
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  LIST_ENTRY           *Node;
  AML_OBJECT_INSTANCE  *Object;

  Node = GetNextNode (ListHead, ListHead);
  while (Node != ListHead) {
    Object = AML_OBJECT_INSTANCE_FROM_LINK (Node);
    // Get next node before freeing current Object
    Node = GetNextNode (ListHead, Node);
    // Free Object
    InternalFreeAmlObject (&Object, ListHead);
  }

  return EFI_SUCCESS;
}

/**
  Validate that ACPI table is completed and return Table and Size

  @param[in,out]  ListHead  - Head of linked list of Objects
  @param[out]     Table     - Completed ACPI Table
  @param[out]     TableSize - Completed ACPI Table size

  @retval         EFI_SUCCESS
                  EFI_INVALID_PARAMETER
                  EFI_DEVICE_ERROR
**/
EFI_STATUS
EFIAPI
AmlGetCompletedTable (
  IN OUT  LIST_ENTRY  *ListHead,
  OUT  VOID           **Table,
  OUT  UINTN          *TableSize
  )
{
  LIST_ENTRY           *Node;
  AML_OBJECT_INSTANCE  *Object;

  if (ListHead == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: ListHead cannot be NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  *Table     = NULL;
  *TableSize = 0;
  Node       = GetFirstNode (ListHead);
  if (!IsNodeAtEnd (ListHead, Node)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Multiple nodes remain, Likely missed an 'AmlClose' call\n", __FUNCTION__));
    return EFI_DEVICE_ERROR;
  } else {
    Object = AML_OBJECT_INSTANCE_FROM_LINK (Node);
    if (!Object->Completed) {
      DEBUG ((DEBUG_ERROR, "%a: ERROR: Final node not completed: Likely missed an 'AmlCLose' call\n", __FUNCTION__));
      return EFI_DEVICE_ERROR;
    }

    *Table     = Object->Data;
    *TableSize = Object->DataSize;
  }

  return EFI_SUCCESS;
}

/**
  Initialize Table List to work with AmlGenerationLib

  Allocates a LIST_ENTRY linked list item and initializes it.  Use
  AmlReleaseTableList to free resulting table and LIST_ENTRY.

  @param[in,out]  ListHead  - Head of linked list of Objects

  @retval         EFI_SUCCESS
                  EFI_INVALID_PARAMETER
                  EFI_OUT_OF_RESOURCES
**/
EFI_STATUS
EFIAPI
AmlInitializeTableList (
  IN OUT  LIST_ENTRY  **ListHead
  )
{
  if (ListHead == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: ListHead = NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  *ListHead = AllocatePool (sizeof (LIST_ENTRY));
  if (*ListHead == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Unable to allocate Table List Head\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (*ListHead);

  return EFI_SUCCESS;
}

/**
  Release table List

  Releases all elements.  Use to free built table and LIST_ENTRY allocated by
  AmlInitializeTableList.

  @param[in,out]  ListHead  - Head of linked list of Objects

  @retval         EFI_SUCCESS
                  EFI_INVALID_PARAMETER
**/
EFI_STATUS
EFIAPI
AmlReleaseTableList (
  IN OUT  LIST_ENTRY  **ListHead
  )
{
  if (*ListHead == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: NULL ListHead passed in\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  AmlFreeObjectList (*ListHead);
  FreePool (*ListHead);
  *ListHead = NULL;

  return EFI_SUCCESS;
}
