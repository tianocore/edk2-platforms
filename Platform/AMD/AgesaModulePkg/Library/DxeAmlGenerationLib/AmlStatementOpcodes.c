/** @file

  Copyright (C) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"
#include <Filecode.h>

#define FILECODE  LIBRARY_DXEAMLGENERATIONLIB_AMLSTATEMENTOPCODES_FILECODE

/**
  Creates an Else object

  TermList must be created between AmlStart and AmlClose Phase

  Since ElseIf (...) is created with Else {( If (){})}.  ElseIf will not be
  supported and must be created with Else and If.

  DefElse := Nothing | <elseop pkglength termlist>
  ElseOp := 0xA1

  EXAMPLE:
  AmlIf (AmlStart, ListHead);
  {
    { // Predicate
      AmlOpDataInteger (1, ListHead);
    } // Predicate
    { // TermList
      ...
    } // TermList
  }
  AmlIf (AmlClose, ListHead);
  AmlElse (AmlStart, ListHead);
  {
    AmlIf (AmlStart, ListHead);
    {
      {} // Predicate
      {} // Termlist
    }
    AmlIf (AmlClose, ListHead);
    AmlElse (AmlStart, ListHead);
    {} // TermList
    AmlElse (AmlClose, ListHead);
  }

  @param[in]      Phase     - Either AmlStart or AmlClose
  @param[in,out]  ListHead  - Linked list has completed AmlElse Object after
                              AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlElse (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, "Else", ListHead);

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: If PkgLength object\n", __func__));
        goto Done;
      }

      // DataRefObject is outside the scope of this object
      break;
    case AmlClose:
      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: close PkgLength object\n", __func__));
        goto Done;
      }

      // DataRefObject should be closed already
      Status = InternalAmlLocateObjectByIdentifier (&Object, "Else", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locating Return Object\n", __func__));
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
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: collecting Child data\n", __func__));
        goto Done;
      }

      // Handle Return with no arguments
      if ((ChildObject->Data == NULL) || (ChildObject->DataSize == 0)) {
        Status = EFI_DEVICE_ERROR;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: If must have at least a Predicate\n", __func__));
        goto Done;
      }

      // Allocate buffer for Return object
      Object->Data     = AllocatePool (ChildObject->DataSize + 1);
      Object->DataSize = ChildObject->DataSize + 1;
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object=Return\n", __func__));
        goto Done;
      }

      // Fill out Return object
      Object->Data[0] = AML_ELSE_OP;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;
      Status            = EFI_SUCCESS;
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
  Creates a If object

  Predicate and TermList must be created between AmlStart and AmlClose Phase

  Since ElseIf (...) is created with Else {( If (){})}.  ElseIf will not be
  supported and must be created with Else and If.

  DefIfElse := IfOp PkgLength Predicate TermList DefElse
  IfOp := 0xA0
  Predicate := TermArg => Integer

  EXAMPLE:
  AmlIf (AmlStart, ListHead);
  {
    { // Predicate
      AmlOpDataInteger (1, ListHead);
    } // Predicate
    { // TermList
      ...
    } // TermList
  }
  AmlIf (AmlClose, ListHead);
  AmlElse (AmlStart, ListHead);
  {
    AmlIf (AmlStart, ListHead);
    {
      {} // Predicate
      {} // Termlist
    }
    AmlIf (AmlClose, ListHead);
    AmlElse (AmlStart, ListHead);
    {} // TermList
    AmlElse (AmlClose, ListHead);
  }

  @param[in]      Phase     - Either AmlStart or AmlClose
  @param[in,out]  ListHead  - Linked list has completed AmlIf Object after
                              AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlIf (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, "If", ListHead);

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: If PkgLength object\n", __func__));
        goto Done;
      }

      // DataRefObject is outside the scope of this object
      break;
    case AmlClose:
      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: close PkgLength object\n", __func__));
        goto Done;
      }

      // DataRefObject should be closed already
      Status = InternalAmlLocateObjectByIdentifier (&Object, "If", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locating Return Object\n", __func__));
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
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: collecting Child data\n", __func__));
        goto Done;
      }

      // Handle Return with no arguments
      if ((ChildObject->Data == NULL) || (ChildObject->DataSize == 0)) {
        Status = EFI_DEVICE_ERROR;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: If must have at least a Predicate\n", __func__));
        goto Done;
      }

      // Allocate buffer for Return object
      Object->Data     = AllocatePool (ChildObject->DataSize + 1);
      Object->DataSize = ChildObject->DataSize + 1;
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object=Return\n", __func__));
        goto Done;
      }

      // Fill out Return object
      Object->Data[0] = AML_IF_OP;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;
      Status            = EFI_SUCCESS;
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
  Creates a Notify object

  DefNotify     := NotifyOp NotifyObject NotifyValue
  NotifyOp      := 0x86
  NotifyObject  := SuperName => ThermalZone | Processor | Device
  NotifyValue   := TermArg => Integer

  @param[in]      NotifyObject  - String of Namestring to a device
  @param[in]      NotifyValue   - Integer Notify value
  @param[in,out]  ListHead      - Linked list updated with Notify object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPNotify (
  IN      CHAR8       *NotifyObject,
  IN      UINT64      NotifyValue,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  Status = InternalAppendNewAmlObject (&Object, NotifyObject, ListHead);
  Status = AmlOPNameString (NotifyObject, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Failed creating NotifyObject NameString\n", __func__));
    goto Done;
  }

  Status = AmlOPDataInteger (NotifyValue, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Failed creating NotifyValue Integer\n", __func__));
    goto Done;
  }

  Status = InternalAmlLocateObjectByIdentifier (&Object, NotifyObject, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: locating Return Object\n", __func__));
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
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: collecting Child data\n", __func__));
    goto Done;
  }

  // Allocate buffer for Return object
  Object->Data     = AllocatePool (ChildObject->DataSize + 1);
  Object->DataSize = ChildObject->DataSize + 1;
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object=Return\n", __func__));
    goto Done;
  }

  // Fill out Return object
  Object->Data[0] = AML_NOTIFY_OP;
  CopyMem (
    &Object->Data[1],
    ChildObject->Data,
    ChildObject->DataSize
    );
  InternalFreeAmlObject (&ChildObject, ListHead);
  Object->Completed = TRUE;
  Status            = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    InternalFreeAmlObject (&ChildObject, ListHead);
  }

  return Status;
}

/**
  Creates a Return object

  Object must be created between AmlStart and AmlClose Phase

  DefReturn  := ReturnOp ArgObject
  ReturnOp   := 0xA4
  ArgObject  := TermArg => DataRefObject

  @param[in]      Phase     - Either AmlStart or AmlClose
  @param[in,out]  ListHead  - Linked list has completed String Object after
                              AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlReturn (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, "Return", ListHead);
      // DataRefObject is outside the scope of this object
      break;
    case AmlClose:
      // DataRefObject should be closed already
      Status = InternalAmlLocateObjectByIdentifier (&Object, "Return", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locating Return Object\n", __func__));
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
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: collecting Child data\n", __func__));
        goto Done;
      }

      // Handle Return with no arguments
      if ((ChildObject->Data == NULL) || (ChildObject->DataSize == 0)) {
        // Return without arguments is treated like Return(0)
        // Zeroed byte = ZeroOp
        ChildObject->Data = AllocateZeroPool (sizeof (UINT8));
        if (ChildObject->Data == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Zero Child for Return\n", __func__));
          goto Done;
        }

        ChildObject->DataSize = 1;
      }

      // Allocate buffer for Return object
      Object->Data     = AllocatePool (ChildObject->DataSize + 1);
      Object->DataSize = ChildObject->DataSize + 1;
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object=Return\n", __func__));
        goto Done;
      }

      // Fill out Return object
      Object->Data[0] = AML_RETURN_OP;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;
      Status            = EFI_SUCCESS;
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
