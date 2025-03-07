/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_AML_OBJECTS_H_
#define INTERNAL_AML_OBJECTS_H_

// #include "LocalAmlLib.h"

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
  );

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
  IN      AML_OBJECT_INSTANCE  **Object,
  IN OUT  LIST_ENTRY           *ListHead
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif // INTERNAL_AML_OBJECTS_H_
