/** @file

  Copyright (C) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"
#include <Filecode.h>

#define FILECODE  LIBRARY_DXEAMLGENERATIONLIB_LOCALAMLOBJECTS_FILECODE

/**
  Free Object->Data

  Frees Object->Data, Nulls pointer, zeros size and marks
  Object->Completed = FALSE

  @param [in]     Object      - Pointer to Object to have Data freed

  @return         EFI_SUCCESS - Object Freed
  @return         <all others> - Object free failed
**/
EFI_STATUS
EFIAPI
InternalFreeAmlObjectData (
  IN      AML_OBJECT_INSTANCE  *Object
  )
{
  if (Object == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Object->Data != NULL) {
    FreePool (Object->Data);
    Object->Data      = NULL;
    Object->DataSize  = 0;
    Object->Completed = FALSE;
  }

  return EFI_SUCCESS;
}

/**
  Free an Object

  Removes Object from it's linked list.
  Frees Object->Data
  Frees Object

  @param [in]     Object      - Pointer to Object to be freed
  @param [in,out] ListHead    - Head of AML Object linked list

  @return         EFI_SUCCESS - Object Freed
  @return         <all others> - Object free failed
**/
EFI_STATUS
EFIAPI
InternalFreeAmlObject (
  IN      AML_OBJECT_INSTANCE  **FreeObject,
  IN OUT  LIST_ENTRY           *ListHead
  )
{
  AML_OBJECT_INSTANCE  *Object;

  if ((FreeObject == NULL) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Object = *FreeObject;
  if (Object != NULL) {
    InternalFreeAmlObjectData (Object);
    if (IsNodeInList (ListHead, &Object->Link)) {
      RemoveEntryList (&Object->Link);
    }

    FreePool (Object);
  }

  *FreeObject = NULL;
  return EFI_SUCCESS;
}

/**
  Creates a new AML_OBJECT_INSTANCE.  Object->Data will be NULL and
  Object->DataSize will be 0

  Allocates AML_OBJECT_INSTANCE which must be freed by caller

  @param [out]    ReturnObject  - Pointer to an Object

  @return         EFI_SUCCESS   - Object created and appended to linked list
  @return         <all others>  - Object creation failed, Object = NULL
**/
EFI_STATUS
EFIAPI
InternalNewAmlObjectNoData (
  OUT  AML_OBJECT_INSTANCE  **ReturnObject
  )
{
  AML_OBJECT_INSTANCE  *Object;

  if (ReturnObject == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ReturnObject = NULL;

  // Allocate AML Object
  Object = AllocateZeroPool (sizeof (AML_OBJECT_INSTANCE));
  if (Object == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Allocate Object Failed\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  Object->DataSize  = 0;
  Object->Data      = NULL;
  Object->Signature = AML_OBJECT_INSTANCE_SIGNATURE;

  *ReturnObject = Object;
  return EFI_SUCCESS;
}

/**
  Inserts a new AML_OBJECT_INSTANCE at the end of the linked list.  Object->Data
  will be NULL and Object->DataSize will be 0

  Allocates AML_OBJECT_INSTANCE which must be freed by caller

  @param [out]    ReturnObject  - Pointer to an Object
  @param [in,out] ListHead      - Head of AML Object linked list

  @return         EFI_SUCCESS   - Object created and appended to linked list
  @return         <all others>  - Object creation failed, Object = NULL
**/
EFI_STATUS
EFIAPI
InternalAppendNewAmlObjectNoData (
  OUT  AML_OBJECT_INSTANCE  **ReturnObject,
  IN OUT  LIST_ENTRY        *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;

  if ((ListHead == NULL) || (ReturnObject == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InternalNewAmlObjectNoData (&Object);
  if (!EFI_ERROR (Status)) {
    InsertTailList (ListHead, &Object->Link);
    *ReturnObject = Object;
  }

  return Status;
}

/**
  Inserts a new AML_OBJECT_INSTANCE at the end of the linked list.  Using a
  string Identifier for comparison purposes

  Allocates AML_OBJECT_INSTANCE which must be freed by caller

  @param [out]    ReturnObject  - Pointer to an Object
  @param [in]     Identifier    - String Identifier to create object with
  @param [in,out] ListHead      - Head of AML Object linked list

  @return         EFI_SUCCESS   - Object created and appended to linked list
  @return         <all others>  - Object creation failed, Object = NULL
**/
EFI_STATUS
EFIAPI
InternalAppendNewAmlObject (
  OUT  AML_OBJECT_INSTANCE  **ReturnObject,
  IN      CHAR8             *Identifier,
  IN OUT  LIST_ENTRY        *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;

  if ((Identifier == NULL) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Object = NULL;

  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Allocate Identifier Data + NULL termination
  Object->DataSize = AsciiStrLen (Identifier) + 1;
  Object->Data     = AllocatePool (Object->DataSize);
  if (Object->Data == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Allocate Data Identifier=%a\n", __func__, Identifier));
    InternalFreeAmlObject (&Object, ListHead);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Object->Data, Identifier, Object->DataSize);

  *ReturnObject = Object;
  return EFI_SUCCESS;
}

/**
  Finds AML_OBJECT_INSTANCE given a string Identifier looking backwards in the
  AML_OBJECT_INSTANCE linked list

  @param [out]    ReturnObject  - Pointer to an Object
  @param [in]     Identifier    - String Identifier to create object with
  @param [in]     ListHead      - Head of AML Object linked list

  @return         EFI_SUCCESS   - Object located and returned
  @return         <all others>  - Object creation failed, Object = NULL
**/
EFI_STATUS
EFIAPI
InternalAmlLocateObjectByIdentifier (
  OUT  AML_OBJECT_INSTANCE  **ReturnObject,
  IN      CHAR8             *Identifier,
  IN      LIST_ENTRY        *ListHead
  )
{
  LIST_ENTRY           *Node;
  AML_OBJECT_INSTANCE  *Object;
  UINTN                IdentifierSize;

  if ((Identifier == NULL) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Object        = NULL;
  *ReturnObject = NULL;

  IdentifierSize = AsciiStrLen (Identifier) + 1;
  // Look Backwards and find Node for this Object
  Node = ListHead;
  do {
    Node   = GetPreviousNode (ListHead, Node);
    Object = AML_OBJECT_INSTANCE_FROM_LINK (Node);

    if (Object->Completed) {
      // Object to be found cannot be completed yet
      continue;
    } else {
      if ((Object->DataSize != 0) &&
          (Object->DataSize == IdentifierSize) &&
          (CompareMem (
             Object->Data,
             Identifier,
             MAX (Object->DataSize, IdentifierSize)
             ) == 0))
      {
        *ReturnObject = Object;
        return EFI_SUCCESS;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: First incomplete Object is not %a.\n",
          __func__,
          Identifier
          ));
        // Object looking for should be the first uncompleted Object.
        return EFI_NOT_FOUND;
      }
    }
  } while (Node != ListHead);

  *ReturnObject = NULL;
  return EFI_NOT_FOUND;
}

/**
  Finds all children of the Link and appends them into a single ObjectData
  buffer of ObjectDataSize

  Allocates AML_OBJECT_INSTANCE and Data which must be freed by caller

  @param [out]    ReturnObject  - Pointer to an Object pointer
  @param [out]    ChildCount    - Count of Child Objects collapsed
  @param [in]     Link          - Linked List Object entry to collect children
  @param [in,out] ListHead      - Head of Object Linked List

  @return         EFI_SUCCESS   - ChildObject created and returned
  @return         <all others>  - Object creation failed, Object = NULL
**/
EFI_STATUS
EFIAPI
InternalAmlCollapseAndReleaseChildren (
  OUT  AML_OBJECT_INSTANCE  **ReturnObject,
  OUT  UINTN                *ChildCount,
  IN      LIST_ENTRY        *Link,
  IN OUT  LIST_ENTRY        *ListHead
  )
{
  EFI_STATUS           Status;
  LIST_ENTRY           *Node;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINT8                *TempBuffer;

  Status = EFI_SUCCESS;
  if ((ReturnObject == NULL) ||
      (ChildCount == NULL) ||
      (Link == NULL) ||
      (ListHead == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *ChildCount = 0;

  Status = InternalNewAmlObjectNoData (&Object);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocating Object Data\n", __func__));
    goto Done;
  }

  // Get first Child Node
  Node = GetNextNode (ListHead, Link);
  while (Node != ListHead) {
    ChildObject = AML_OBJECT_INSTANCE_FROM_LINK (Node);
    // Expand data buffer to fit existing data + new data
    TempBuffer = ReallocatePool (
                   Object->DataSize,
                   Object->DataSize + ChildObject->DataSize,
                   Object->Data
                   );
    if (TempBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      FreePool (Object->Data);
      Object->Data = NULL;
      DEBUG ((DEBUG_ERROR, "%a: ERROR: reallocating Object Data\n", __func__));
      goto Done;
    }

    Object->Data = TempBuffer;
    // Copy new data at end of buffer
    CopyMem (
      &Object->Data[Object->DataSize],
      ChildObject->Data,
      ChildObject->DataSize
      );
    Object->DataSize += ChildObject->DataSize;
    // Get Next ChildObject Node, then free ChildObject from list
    Node = GetNextNode (ListHead, Node);
    InternalFreeAmlObject (&ChildObject, ListHead);
    *ChildCount = *ChildCount + 1;
  }

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    Object = NULL;
  }

  *ReturnObject = Object;
  return Status;
}
