/** @file

  Copyright (C) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"
#include <Filecode.h>

#define FILECODE  LIBRARY_DXEAMLGENERATIONLIB_AMLNAMESPACEMODIFIEROBJECTS_FILECODE

/**
  Creates a Scope (ObjectName, Object)

  Object must be created between AmlStart and AmlClose Phase

  DefScope  := ScopeOp PkgLength NameString TermList
  ScopeOp   := 0x10

  @param[in]      Phase     - Either AmlStart or AmlClose
  @param[in]      String    - Location
  @param[in,out]  ListHead  - Linked list has completed String Object after
                              AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlScope (
  IN      AML_FUNCTION_PHASE  Phase,
  IN      CHAR8               *String,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((Phase >= AmlInvalid) || (String == NULL) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start %a object\n", __func__, String));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start PkgLength for %a object\n", __func__, String));
        goto Done;
      }

      // Insert required NameString
      Status = AmlOPNameString (String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Insert NameString for %a object\n", __func__, String));
        goto Done;
      }

      // TermList is too complicated and must be added outside
      break;

    case AmlClose:
      // TermList should be closed already

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Close PkgLength for %a object\n", __func__, String));
        goto Done;
      }

      Status = InternalAmlLocateObjectByIdentifier (&Object, String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __func__, String));
        goto Done;
      }

      // Get rid of original Identifier data
      InternalFreeAmlObjectData (Object);
      // Collect child data and delete children
      Status = InternalAmlCollapseAndReleaseChildren (
                 &ChildObject,
                 &ChildCount,
                 &Object->Link,
                 ListHead
                 );
      if (EFI_ERROR (Status) ||
          (ChildObject->Data == NULL) ||
          (ChildObject->DataSize == 0))
      {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a has no child data.\n", __func__, String));
        goto Done;
      }

      // Scope Op is one byte
      Object->DataSize = ChildObject->DataSize + 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __func__, String));
        goto Done;
      }

      Object->Data[0] = AML_SCOPE_OP;
      CopyMem (&Object->Data[1], ChildObject->Data, ChildObject->DataSize);
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;

      Status = EFI_SUCCESS;
      break;

    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    InternalFreeAmlObject (&ChildObject, ListHead);
  }

  return Status;
}

/**
  Creates a Name (ObjectName, Object)

  Object must be created between AmlStart and AmlClose Phase

  DefName  := NameOp NameString ChildObjectData
  NameOp   := 0x08

  @param[in]      Phase     - Either AmlStart or AmlClose
  @param[in]      String    - Object name
  @param[in,out]  ListHead  - Linked list has completed String Object after
                              AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlName (
  IN      AML_FUNCTION_PHASE  Phase,
  IN      CHAR8               *String,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((Phase >= AmlInvalid) || (String == NULL) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start %a object\n", __func__, String));
        goto Done;
      }

      // Insert required NameString
      Status = AmlOPNameString (String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString for %a object\n", __func__, String));
        goto Done;
      }

      break;
    case AmlClose:
      // DataRefObject should be closed already

      Status = InternalAmlLocateObjectByIdentifier (&Object, String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __func__, String));
        goto Done;
      }

      // Get rid of original Identifier data
      InternalFreeAmlObjectData (Object);
      // Collect child data and delete children
      Status = InternalAmlCollapseAndReleaseChildren (
                 &ChildObject,
                 &ChildCount,
                 &Object->Link,
                 ListHead
                 );
      if (EFI_ERROR (Status) ||
          (ChildObject->Data == NULL) ||
          (ChildObject->DataSize == 0))
      {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a has no child data.\n", __func__, String));
        goto Done;
      }

      Object->Data = AllocatePool (ChildObject->DataSize + 1);
      // Name Op is one byte
      Object->DataSize = ChildObject->DataSize + 1;
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __func__, String));
        goto Done;
      }

      Object->Data[0] = AML_NAME_OP;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;

      Status = EFI_SUCCESS;
      break;

    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    InternalFreeAmlObject (&ChildObject, ListHead);
  }

  return Status;
}

/**
  Creates an Alias (SourceObject, AliasObject)

  DefAlias  := AliasOp NameString NameString
  AliasOp   := 0x06

  @param[in]      SourceName - Any named Source Object NameString
  @param[in]      AliasName  - Alias Object NameString
  @param[in,out]  ListHead   - Linked list has completed the Alias Object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPAlias (
  IN      CHAR8       *SourceName,
  IN      CHAR8       *AliasName,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((SourceName == NULL) || (AliasName == NULL) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  // Start ALIAS object
  Status = InternalAppendNewAmlObject (&Object, "ALIAS", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot append ALIAS object\n", __func__));
    goto Done;
  }

  // Insert required Object (to be aliased) NameString
  Status = AmlOPNameString (SourceName, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot create NameString: %a \n", __func__, SourceName));
    goto Done;
  }

  // Insert required Alias NameString
  Status = AmlOPNameString (AliasName, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot create NameString: %a \n", __func__, AliasName));
    goto Done;
  }

  Status = InternalAmlLocateObjectByIdentifier (&Object, "ALIAS", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot locate ALIAS object\n", __func__));
    goto Done;
  }

  // Get rid of original Identifier data
  InternalFreeAmlObjectData (Object);
  // Collect child data and delete children
  Status = InternalAmlCollapseAndReleaseChildren (
             &ChildObject,
             &ChildCount,
             &Object->Link,
             ListHead
             );
  if (EFI_ERROR (Status) ||
      (ChildObject->Data == NULL) ||
      (ChildObject->DataSize == 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: %a has no child data.\n", __func__, SourceName));
    goto Done;
  }

  Object->Data = AllocateZeroPool (ChildObject->DataSize + 1);
  // Alias Op is one byte
  Object->DataSize = ChildObject->DataSize + 1;
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __func__, SourceName));
    goto Done;
  }

  Object->Data[0] = AML_ALIAS_OP;
  CopyMem (
    &Object->Data[1],
    ChildObject->Data,
    ChildObject->DataSize
    );

  InternalFreeAmlObject (&ChildObject, ListHead);
  Object->Completed = TRUE;

  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    InternalFreeAmlObject (&ChildObject, ListHead);
  }

  return Status;
}
