/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"

// ----------------------------------------------------------------------------
//  Expression Opcodes Encoding
// ----------------------------------------------------------------------------
//   ExpressionOpcode := DefAcquire | DefAdd | DefAnd | DefBuffer | DefConcat |
//     DefConcatRes | DefCondRefOf | DefCopyObject | DefDecrement |
//     DefDerefOf | DefDivide | DefFindSetLeftBit | DefFindSetRightBit |
//     DefFromBCD | DefIncrement | DefIndex | DefLAnd | DefLEqual |
//     DefLGreater | DefLGreaterEqual | DefLLess | DefLLessEqual | DefMid |
//     DefLNot | DefLNotEqual | DefLoadTable | DefLOr | DefMatch | DefMod |
//     DefMultiply | DefNAnd | DefNOr | DefNot | DefObjectType | DefOr |
//     DefPackage | DefVarPackage | DefRefOf | DefShiftLeft | DefShiftRight |
//     DefSizeOf | DefStore | DefSubtract | DefTimer | DefToBCD | DefToBuffer |
//     DefToDecimalString | DefToHexString | DefToInteger | DefToString |
//     DefWait | DefXOr | MethodInvocation
// ----------------------------------------------------------------------------

/**
  Creates a  Buffer (BufferSize) {Initializer} => Buffer Object

  Initializers must be created between AmlStart and AmlClose Phase

  DefBuffer   := BufferOp PkgLength BufferSize ByteList
  BufferOp    := 0x11
  BufferSize  := TermArg => Integer

  @param[in]      Phase       - Either AmlStart or AmlClose
  @param[in]      BufferSize  - Requested BufferSize, Encoded value will be
                                MAX (BufferSize OR Child->DataSize)
  @param[in,out]  ListHead    - Linked list has completed Buffer Object after
                                AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlBuffer (
  IN      AML_FUNCTION_PHASE  Phase,
  IN      UINTN               BufferSize,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;
  UINTN                InternalBufferSize;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // iASL compiler 20200110 only keeps lower 32 bits of size.  We'll error if
  // someone requests something >= 4GB size.  Have a message with this to be
  // very specific
  if (BufferSize >= SIZE_4GB) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: BufferSize=0x%X >= 4GB\n", __FUNCTION__, BufferSize));
    return EFI_INVALID_PARAMETER;
  }

  switch (Phase) {
    case AmlStart:
      // Start the Buffer Object
      Status = InternalAppendNewAmlObject (&Object, "BUFFER", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start BUFFER object\n", __FUNCTION__));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Buffer PkgLength object\n", __FUNCTION__));
        goto Done;
      }

      // Start BufferSize
      Status = InternalAppendNewAmlObject (&Object, "BUFFERSIZE", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start BUFFERSIZE object\n", __FUNCTION__));
        goto Done;
      }

      // ByteList is too complicated and must be added outside
      break;

    case AmlClose:
      // ByteList items should be closed already

      // Close BufferSize
      Status = InternalAmlLocateObjectByIdentifier (
                 &Object,
                 "BUFFERSIZE",
                 ListHead
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Fail locate BufferSize object\n", __FUNCTION__));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: collect BufferSize children\n", __FUNCTION__));
        goto Done;
      }

      // Set BufferSize Object to correct value and size.
      // BufferSize should be from zero (no Child Data) to MAX of requested
      // BufferSize or size required for ChildObject->Data.
      InternalBufferSize = MAX (BufferSize, ChildObject->DataSize);
      // iASL compiler 20200110 only keeps lower 32 bits of size.  We'll error if
      // someone requests something >= 4GB size.
      if (InternalBufferSize >= SIZE_4GB) {
        Status = EFI_BAD_BUFFER_SIZE;
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: BufferSize 0x%X >= 4GB\n",
          __FUNCTION__,
          InternalBufferSize
          ));
        goto Done;
      }

      Status = InternalAmlDataIntegerBuffer (
                 InternalBufferSize,
                 (VOID **)&Object->Data,
                 &Object->DataSize
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: calc BufferSize\n", __FUNCTION__));
        goto Done;
      }

      if ((ChildObject->DataSize != 0) && (ChildObject->Data != NULL)) {
        // Make room for ChildObject->Data
        Object->Data = ReallocatePool (
                         Object->DataSize,
                         Object->DataSize +
                         ChildObject->DataSize,
                         Object->Data
                         );
        if (Object->Data == NULL) {
          DEBUG ((DEBUG_ERROR, "%a: ERROR: to reallocate BufferSize\n", __FUNCTION__));
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        CopyMem (
          &Object->Data[Object->DataSize],
          ChildObject->Data,
          ChildObject->DataSize
          );
        Object->DataSize += ChildObject->DataSize;
      }

      // Free Child Object since it has been consumed
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: close PkgLength object\n", __FUNCTION__));
        goto Done;
      }

      // Complete Buffer object with all data
      Status = InternalAmlLocateObjectByIdentifier (&Object, "BUFFER", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locate Buffer object\n", __FUNCTION__));
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

      // Buffer must have at least PkgLength BufferSize
      if (EFI_ERROR (Status) ||
          (ChildObject->Data == NULL) ||
          (ChildObject->DataSize == 0))
      {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: No Buffer Data\n", __FUNCTION__));
        goto Done;
      }

      //  BufferOp is one byte
      Object->DataSize = ChildObject->DataSize + 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Buffer allocate failed\n", __FUNCTION__));
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      Object->Data[0] = AML_BUFFER_OP;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      // Free Child Object since it has been consumed
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates a LEqual (Source1, Source2) => Boolean

  Source1 and Source2 operands must be created between AmlStart and AmlClose Phase

  DefLEqual := LequalOp Operand Operand
  LequalOp  := 0x93
  Operand   := TermArg => Integer

  @param[in]      Phase       - Either AmlStart or AmlClose
  @param[in,out]  ListHead    - Linked list has completed LEqual Object after
                                AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlLEqual (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Phase) {
    case AmlStart:
      // Start the LEqual Object
      Status = InternalAppendNewAmlObject (&Object, "LEQUAL", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start LEQUAL object\n", __FUNCTION__));
        goto Done;
      }

      // Operands are too complicated and must be added outside
      break;

    case AmlClose:

      // Close LEqual
      Status = InternalAmlLocateObjectByIdentifier (
                 &Object,
                 "LEQUAL",
                 ListHead
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Fail locate LEqual object\n", __FUNCTION__));
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

      // LEqual must have at least two operands
      if (EFI_ERROR (Status) ||
          (ChildObject->Data == NULL) ||
          (ChildObject->DataSize == 0))
      {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: No LEqual Args\n", __FUNCTION__));
        goto Done;
      }

      //  LequalOp is one byte
      Object->DataSize = ChildObject->DataSize + 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Buffer allocate failed\n", __FUNCTION__));
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      Object->Data[0] = AML_LEQUAL_OP;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      // Free Child Object since it has been consumed
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/*
  Creates (NumElements) section of a Package: {PackageList} => Package

  Initializers must be created between AmlStart and AmlClose Phase
  Internal only function no public reference or documentation needed.

  NumElements        := ByteData
  VarNumElements     := TermArg => Integer
  PackageElementList := Nothing | <PackageElement PackageElementList>
  PackageElement     := DataRefObject | NameString

  @param[in]      Phase       - Either AmlStart or AmlClose
  @param[in,out]  NumElements - Number of elements in the package. If 0, size
                                is calculated from the PackageList.
  @param[in,out]  ListHead    - Linked list has completed Package Object after
                                AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
*/
EFI_STATUS
EFIAPI
InternalAmlNumElements (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  UINTN               *NumElements,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;
  ChildCount  = 0;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Phase) {
    case AmlStart:
      // Start Number of Elements Object
      Status = InternalAppendNewAmlObject (&Object, "NUM_ELEMENTS", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NUM_ELEMENTS object\n", __FUNCTION__));
        goto Done;
      }

      // PackageList is too complicated and must be added outside
      break;

    case AmlClose:
      // PackageList items should be closed already

      // Close Number of Elements Object
      Status = InternalAmlLocateObjectByIdentifier (
                 &Object,
                 "NUM_ELEMENTS",
                 ListHead
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Fail locate NUM_ELEMENTS object\n", __FUNCTION__));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: collect NUM_ELEMENTS children\n", __FUNCTION__));
        goto Done;
      }

      // We do not have to change anything for NumElements >= Child Count
      if (*NumElements == 0) {
        *NumElements = ChildCount;
      } else if (*NumElements < ChildCount) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: NumElements < ChildCount.\n", __FUNCTION__));
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      if (*NumElements <= MAX_UINT8) {
        Object->DataSize = 1;
        Object->Data     = AllocateZeroPool (Object->DataSize);
        if (Object->Data == NULL) {
          DEBUG ((DEBUG_ERROR, "%a: ERROR: NumElements allocate failed\n", __FUNCTION__));
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        Object->Data[0] = (UINT8)*NumElements;
      } else {
        Status = InternalAmlDataIntegerBuffer (
                   *NumElements,
                   (VOID **)&Object->Data,
                   &Object->DataSize
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: ERROR: calc NumElements\n", __FUNCTION__));
          goto Done;
        }
      }

      if ((ChildObject->DataSize != 0) && (ChildObject->Data != NULL)) {
        // Make room for ChildObject->Data
        Object->Data = ReallocatePool (
                         Object->DataSize,
                         Object->DataSize +
                         ChildObject->DataSize,
                         Object->Data
                         );
        if (Object->Data == NULL) {
          DEBUG ((DEBUG_ERROR, "%a: ERROR: to reallocate NumElements\n", __FUNCTION__));
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        CopyMem (
          &Object->Data[Object->DataSize],
          ChildObject->Data,
          ChildObject->DataSize
          );
        Object->DataSize += ChildObject->DataSize;
      }

      // Free Child Object since it has been consumed
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates a  Package (NumElements) {PackageList} => Package

  Initializers must be created between AmlStart and AmlClose Phase

  DefPackage         := PackageOp PkgLength NumElements PackageElementList
  PackageOp          := 0x12
  DefVarPackage      := VarPackageOp PkgLength VarNumElements PackageElementList
  VarPackageOp       := 0x13
  NumElements        := ByteData
  VarNumElements     := TermArg => Integer
  PackageElementList := Nothing | <PackageElement PackageElementList>
  PackageElement     := DataRefObject | NameString

  @param[in]      Phase       - Either AmlStart or AmlClose
  @param[in]      NumElements - Number of elements in the package. If 0, size
                                is calculated from the PackageList.
  @param[in,out]  ListHead    - Linked list has completed Package Object after
                                AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlPackage (
  IN      AML_FUNCTION_PHASE  Phase,
  IN      UINTN               NumElements,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;
  UINT8                OpCode;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Phase) {
    case AmlStart:
      // Start the Package Object
      Status = InternalAppendNewAmlObject (&Object, "PACKAGE", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start PACKAGE object\n", __FUNCTION__));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Package PkgLength object\n", __FUNCTION__));
        goto Done;
      }

      // Start Number of Elements Object
      Status = InternalAmlNumElements (AmlStart, &NumElements, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NUM_ELEMENTS object\n", __FUNCTION__));
        goto Done;
      }

      // PackageList is too complicated and must be added outside
      break;

    case AmlClose:
      // PackageList items should be closed already

      // Close Number of Elements Object
      Status = InternalAmlNumElements (AmlClose, &NumElements, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Close NUM_ELEMENTS object\n", __FUNCTION__));
        goto Done;
      }

      if (NumElements <= MAX_UINT8) {
        OpCode = AML_PACKAGE_OP;
      } else {
        OpCode = AML_VAR_PACKAGE_OP;
      }

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: close PkgLength object\n", __FUNCTION__));
        goto Done;
      }

      // Complete Package object with all data
      Status = InternalAmlLocateObjectByIdentifier (&Object, "PACKAGE", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locate PACKAGE object\n", __FUNCTION__));
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
      // Package must have at least PkgLength NumElements
      if (EFI_ERROR (Status) ||
          (ChildObject->Data == NULL) ||
          (ChildObject->DataSize == 0))
      {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: No Package Data\n", __FUNCTION__));
        goto Done;
      }

      //  PackageOp and VarPackageOp are both one byte
      Object->DataSize = ChildObject->DataSize + 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Package allocate failed\n", __FUNCTION__));
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      Object->Data[0] = OpCode;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      // Free Child Object since it has been consumed
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates a Store expression

  Syntax:
  Store (Source, Destination) => DataRefObject Destination = Source => DataRefObject

  Store expression must be created between AmlStart and AmlClose Phase.

  DefStore := StoreOp TermArg SuperName
  StoreOp := 0x70

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlStore (
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
      // Start Store expression
      Status = InternalAppendNewAmlObject (&Object, "STORE", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot append STORE object\n", __FUNCTION__));
        goto Done;
      }

      // TermArg and SuperName are outside the scope of this object.  They must be
      // defined as part of a multi-tier call - in between AmlStore(AmlStart,..) and
      // AmlStore(AmlClose,...) - when creating the Store expression.
      break;

    case AmlClose:
      // TermArg and SuperName must have been created and closed by now.
      Status = InternalAmlLocateObjectByIdentifier (&Object, "STORE", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locating STORE Object\n", __FUNCTION__));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Store() has no child data.\n", __FUNCTION__));
        goto Done;
      }

      Object->Data = AllocateZeroPool (ChildObject->DataSize + 1);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for Store()\n", __FUNCTION__));
        goto Done;
      }

      // Store Op is one byte
      Object->DataSize = ChildObject->DataSize + 1;

      // Fill out Store object
      Object->Data[0] = AML_STORE_OP;
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
  Creates a Shift Left or Right expression

  Syntax:
  ShiftLeft (Source, ShiftCount, Reult) => Integer
  Result = Source << ShiftCount => Integer
  Result <<= ShiftCount => Integer

  ShiftRight (Source, ShiftCount, Reult) => Integer
  Result = Source >> ShiftCount => Integer
  Result >>= ShiftCount => Integer

  Shift expression must be created between AmlStart and AmlClose Phase.

  DefShiftLeft := ShiftOp Operand ShiftCount Target
  ShiftOp  := 0x79 or 0x7A
  ShiftCount   := TermArg => Integer

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in]      ShiftOp         - Specifies whether to shift left or shift
                                    right.
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlShift (
  IN      AML_FUNCTION_PHASE  Phase,
  IN      UINT8               ShiftOp,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Phase) {
    case AmlStart:
      // Start Store expression
      Status = InternalAppendNewAmlObject (&Object, "SHIFT", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot append SHIFT object\n", __FUNCTION__));
        goto Done;
      }

      // Operand, ShiftCount, and Target are outside the scope of this object.  They must be
      // defined as part of a multi-tier call - in between AmlShift(AmlStart,..) and
      // AmlShift(AmlClose,...) - when creating the Shift expression.

      break;

    case AmlClose:
      // Operand, ShiftCount, and Target must have been created and closed by now.
      Status = InternalAmlLocateObjectByIdentifier (&Object, "SHIFT", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locating SHIFT Object\n", __FUNCTION__));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Store() has no child data.\n", __FUNCTION__));
        goto Done;
      }

      Object->Data = AllocateZeroPool (ChildObject->DataSize + 1);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for Store()\n", __FUNCTION__));
        goto Done;
      }

      // Store Op is one byte
      Object->DataSize = ChildObject->DataSize + 1;

      // Fill out Store object
      Object->Data[0] = ShiftOp;
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
  Creates a Shift Left expression

  Syntax:
  ShiftLeft (Source, ShiftCount, Result) => Integer
  Result = Source << ShiftCount => Integer
  Result <<= ShiftCount => Integer

  Shift expression must be created between AmlStart and AmlClose Phase.

  DefShiftLeft := ShiftLeftOp Operand ShiftCount Target
  ShiftLeftOp  := 0x79
  ShiftCount   := TermArg => Integer

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlShiftLeft (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlShift (Phase, AML_SHIFT_LEFT_OP, ListHead);
  return Status;
}

/**
  Creates a Shift Right expression

  Syntax:
  ShiftRight (Source, ShiftCount, Reult) => Integer
  Result = Source >> ShiftCount => Integer
  Result >>= ShiftCount => Integer

  Shift expression must be created between AmlStart and AmlClose Phase.

  DefShiftRight := ShiftRightOp Operand ShiftCount Target
  ShiftRightOp  := 0x7A
  ShiftCount   := TermArg => Integer

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlShiftRight (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlShift (Phase, AML_SHIFT_RIGHT_OP, ListHead);
  return Status;
}

/**
  Creates a Find First Set Bit AML object for
  both right and left searches.

  Syntax:
  FindSetLeftBit (Source, Result) => Integer

  FindSetRightBit (Source, Result) => Integer

  Bit Fields must be created between AmlStart and AmlClose Phase.

  DefFindSetLeftBit := FindSetLeftBitOp Operand Target
  FindSetLeftBitOp := 0x81

  DefFindSetRightBit := FindSetRightBitOp Operand Target
  FindSetRightBitOp := 0x82

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in]      FindSetOp       - Specifies whether to search left or search
                                    right.
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlFindSetBit (
  IN      AML_FUNCTION_PHASE  Phase,
  IN      UINT8               FindSetOp,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Phase) {
    case AmlStart:
      // Start Store expression
      Status = InternalAppendNewAmlObject (&Object, "FINDSET", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot append FIND_SET object\n", __FUNCTION__));
        goto Done;
      }

      // Source and Result are outside the scope of this object.  They must be
      // defined as part of a multi-tier call - in between AmlFindSet(AmlStart,..) and
      // AmlFindSet(AmlClose,...) - when creating the FindSetBit expression.

      break;

    case AmlClose:
      // Source and Result must have been created and closed by now.
      Status = InternalAmlLocateObjectByIdentifier (&Object, "FINDSET", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locating FIND_SET Object\n", __FUNCTION__));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Store() has no child data.\n", __FUNCTION__));
        goto Done;
      }

      Object->Data = AllocateZeroPool (ChildObject->DataSize + 1);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for Store()\n", __FUNCTION__));
        goto Done;
      }

      // Store Op is one byte
      Object->DataSize = ChildObject->DataSize + 1;

      // Fill out Store object
      Object->Data[0] = FindSetOp;
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
  Creates a FindSetLeftBit AML Object

  Syntax:
  FindSetLeftBit (Source, Result) => Integer

  FindSetLeftBit expression must be created between
  AmlStart and AmlClose Phase.

  DefFindSetLeftBit := FindSetLeftBitOp Operand Target
  FindSetLeftBitOp := 0x81

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlFindSetLeftBit (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlFindSetBit (Phase, AML_FIND_SET_LEFT_BIT_OP, ListHead);
  return Status;
}

/**
  Creates a FindSetRightBit AML Object

  Syntax:
  FindSetRightBit (Source, Result) => Integer

  FindSetRightBit expression must be created between
  AmlStart and AmlClose Phase.

  DefFindSetRightBit := FindSetRightBit Operand Target
  FindSetRightBit := 0x82

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlFindSetRightBit (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlFindSetBit (Phase, AML_FIND_SET_RIGHT_BIT_OP, ListHead);
  return Status;
}

/**
  Creates a Decrement expression

  Syntax:
  Decrement (Minuend) => Integer
  Minuend-- => Integer

  Creates object to decrement Minuend.

  DefDecrement := DecrementOp SuperName
  DecrementOp := 0x76

  @param[in]      Phase           - Either AmlStart or AmlClose
  @param[in,out]  ListHead        - Linked list has completed String Object after
                                    AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlDecrement (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Phase) {
    case AmlStart:
      // Start decrement expression
      Status = InternalAppendNewAmlObject (&Object, "DECREMENT", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Cannot append DECREMENT object\n", __FUNCTION__));
        goto Done;
      }

      // Minuend is outside the scope of this object.  It must be
      // defined as part of a multi-tier call - in between AmlDecrement(AmlStart,..) and
      // AmlDecrement(AmlClose,...) - when creating the Decrement expression.

      break;

    case AmlClose:
      // Minuend must created and closed by now.
      Status = InternalAmlLocateObjectByIdentifier (&Object, "DECREMENT", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locating DECREMENT Object\n", __FUNCTION__));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Store() has no child data.\n", __FUNCTION__));
        goto Done;
      }

      Object->Data = AllocateZeroPool (ChildObject->DataSize + 1);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for Store()\n", __FUNCTION__));
        goto Done;
      }

      // Decrement Op is one byte
      Object->DataSize = ChildObject->DataSize + 1;

      // Fill out Decrement object
      Object->Data[0] = AML_DECREMENT_OP;
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
