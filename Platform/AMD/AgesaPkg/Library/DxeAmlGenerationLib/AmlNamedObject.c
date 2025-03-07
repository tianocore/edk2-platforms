/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"

#define    METHOD_ARGS_MAX           7
#define    MAX_SYNC_LEVEL            0x0F
#define    GENERIC_FIELD_IDENTIFIER  "FIELD"

/**
  Creates a Device (ObjectName, Object)

  Object must be created between AmlStart and AmlClose Phase

  DefName  := DeviceOp PkgLength NameString TermList
  NameOp   := ExtOpPrefix 0x82
  ExtOpPrefix  := 0x5B

  @param[in]      Phase     - Either AmlStart or AmlClose
  @param[in]      String    - Object name
  @param[in,out]  ListHead  - Linked list has completed String Object after
                              AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlDevice (
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start Device for %a object\n", __FUNCTION__, String));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start PkgLength for %a object\n", __FUNCTION__, String));
        goto Done;
      }

      // Insert required NameString
      Status = AmlOPNameString (String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Insert NameString for %a object\n", __FUNCTION__, String));
        goto Done;
      }

      // TermList is too complicated and must be added outside
      break;

    case AmlClose:
      // TermList should be closed already

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Close PkgLength for %a object\n", __FUNCTION__, String));
        goto Done;
      }

      Status = InternalAmlLocateObjectByIdentifier (&Object, String, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, String));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, String));
        goto Done;
      }

      // Device Op is two bytes
      Object->DataSize = ChildObject->DataSize + 2;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, String));
        goto Done;
      }

      Object->Data[0] = AML_EXT_OP;
      Object->Data[1] = AML_EXT_DEVICE_OP;
      CopyMem (
        &Object->Data[2],
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
  Creates an AccessField

  AccessField := 0x01 AccessType AccessAttrib

  @param[in]      AccessType        - Access type for field member
  @param[in]      AccessAttribute   - Access attribute for field member
  @param[in,out]  ListHead          - Linked list containing AML objects

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlAccessField (
  IN      EFI_ACPI_FIELD_ACCESS_TYPE_KEYWORDS       AccessType,
  IN      EFI_ACPI_FIELD_ACCESS_ATTRIBUTE_KEYWORDS  AccessAttribute,
  IN OUT  LIST_ENTRY                                *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;

  Status = EFI_DEVICE_ERROR;
  Object = NULL;

  // Start new AML object
  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Start ACCESSFIELD object\n", __FUNCTION__));
    goto Done;
  }

  Object->Data = AllocateZeroPool (3);
  // AML_ACCESSFIELD_OP + AccessType + AccessAttrib
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for ACCESSFIELD\n", __FUNCTION__));
    goto Done;
  }

  Object->Data[0]  = AML_FIELD_ACCESS_OP;
  Object->Data[1]  = (UINT8)AccessType;
  Object->Data[2]  = (UINT8)AccessAttribute;
  Object->DataSize = 3;

  Object->Completed = TRUE;
  Status            = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates an ExtendedAccessField

  ExtendedAccessField := 0x03 AccessType ExtendedAccessAttrib AccessLength

  @param[in]      AccessType        - Access type for field member
  @param[in]      AccessAttribute   - Access attribute for field member
  @param[in]      AccessLength      - Specifies the access length for the field member
  @param[in,out]  ListHead          - Linked list containing AML objects

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlExtendedAccessField (
  IN      EFI_ACPI_FIELD_ACCESS_TYPE_KEYWORDS       AccessType,
  IN      EFI_ACPI_FIELD_ACCESS_ATTRIBUTE_KEYWORDS  AccessAttribute,
  IN      UINT8                                     AccessLength,
  IN OUT  LIST_ENTRY                                *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;

  Status = EFI_DEVICE_ERROR;
  Object = NULL;

  // Start new AML object
  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Start EXTENDEDACCESSFIELD object\n", __FUNCTION__));
    goto Done;
  }

  Object->Data = AllocateZeroPool (4);
  // AML_EXTACCESSFIELD_OP + AccessType + AccessAttrib + AccessLength
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for EXTENDEDACCESSFIELD\n", __FUNCTION__));
    goto Done;
  }

  Object->Data[0]  = AML_FIELD_EXT_ACCESS_OP;
  Object->Data[1]  = (UINT8)AccessType;
  Object->Data[2]  = (UINT8)AccessAttribute;
  Object->Data[3]  = AccessLength;
  Object->DataSize = 4;

  Object->Completed = TRUE;
  Status            = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates an AccessAs Field Unit

  AccessAs (AccessType, AccessAttribute)
  AccessAs (AccessType, AccessAttribute (AccessLength))

  AccessField := 0x01 AccessType AccessAttrib
  AccessType := ByteData  // Bits 0:3 - Same as AccessType bits of FieldFlags.
                          // Bits 4:5 - Reserved
                          // Bits 7:6 - 0 = AccessAttrib = Normal Access Attributes
                          // 1 = AccessAttrib = AttribBytes (x)
                          // 2 = AccessAttrib = AttribRawBytes (x)
                          // 3 = AccessAttrib = AttribRawProcessBytes (x)

                          // x' is encoded as bits 0:7 of the AccessAttrib byte.
                          The description of bits 7:6 is incorrect and if AttribBytes,
                          AttribRawBytes, or AttribRawProcessBytes are used here, an
                          ExtendedAccessField is used with the following definitions
  ExtendedAccessField := 0x03 AccessType ExtendedAccessAttrib AccessLength
  ExtendedAccessAttrib := ByteData // 0x0B AttribBytes
                                   // 0x0E AttribRawBytes
                                   // 0x0F AttribRawProcess

  AccessAttrib := ByteData  // If AccessType is BufferAcc for the SMB or
                            // GPIO OpRegions, AccessAttrib can be one of
                            // the following values:
                            // 0x02 AttribQuick
                            // 0x04 AttribSendReceive
                            // 0x06 AttribByte
                            // 0x08 AttribWord
                            // 0x0A AttribBlock
                            // 0x0C AttribProcessCall
                            // 0x0D AttribBlockProcessCall

  @param[in]      AccessType        - Access type for field member
  @param[in]      AccessAttribute   - Access attribute for field member
  @param[in]      AccessLength      - Only used if AccessAttribute is AttribBytes,
                                      AttribRawBytes, or AttribRawProcessBytes.
                                      Specifies the access length for the field member
                                      Otherwise, ignored.
  @param[in,out]  ListHead          - Linked list containing AML objects

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPAccessAs (
  IN      EFI_ACPI_FIELD_ACCESS_TYPE_KEYWORDS       AccessType,
  IN      EFI_ACPI_FIELD_ACCESS_ATTRIBUTE_KEYWORDS  AccessAttribute,
  IN      UINT8                                     AccessLength,
  IN OUT  LIST_ENTRY                                *ListHead
  )
{
  EFI_STATUS  Status;

  // AcessType parameter check
  if (AccessType > BufferAcc) {
    return EFI_INVALID_PARAMETER;
  }

  // AccessAttrib parameter checking
  if ((AccessAttribute >= AttribNormal) && (AccessAttribute <= AttribBlock)) {
    if ((AccessAttribute & 1) == 1) {
      return EFI_INVALID_PARAMETER;
    }
  } else if (AccessAttribute > AttribRawProcessBytes) {
    return EFI_INVALID_PARAMETER;
  }

  // if AccessAttrib requires a length parameter, then an ExtendedAccessField is used
  switch (AccessAttribute) {
    case AttribBytes:
    case AttribRawBytes:
    case AttribRawProcessBytes:
      Status = InternalAmlExtendedAccessField (AccessType, AccessAttribute, AccessLength, ListHead);
      break;
    default:
      Status = InternalAmlAccessField (AccessType, AccessAttribute, ListHead);
      break;
  }

  return Status;
}

/**
  Creates an External Object

  External (ObjectName, ObjectType, ReturnType, ParameterTypes)

  Note: ReturnType is not used for AML encoding and is therefore not passed in
        to this function.
        ParameterTypes is only used if the ObjectType is a MethodObj. It
        specifies MethodObj's argument types in a list.  For the purposes of
        this library, we are passing in the the number of input parameters for
        that MethodObj.

  DefExternal    := ExternalOp NameString ObjectType ArgumentCount
  ExternalOp     := 0x15
  ObjectType     := ByteData
  ArgumentCount  := ByteData (0 - 7)

  @param[in]      Name        - Object name
  @param[in]      ObjectType  - Type of object declared
  @param[in]      NumArgs     - Only used if ObjectType is MethodObj.
                                Specifies the number of input parameters for
                                that MethodObj.
                                Otherwise, ignored.
  @param[in,out]  ListHead    - Linked list that has completed External Object
                                after AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPExternal (
  IN      CHAR8       *Name,
  IN      UINT8       ObjectType,
  IN      UINT8       NumArgs,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  if ((Name == NULL) ||
      (NumArgs > METHOD_ARGS_MAX) ||
      (ObjectType >= InvalidObj) ||
      (ListHead == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  // Start EXTERNAL object
  Status = InternalAppendNewAmlObject (&Object, "EXTERNAL", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Start %a object\n", __FUNCTION__, Name));
    goto Done;
  }

  // Insert required NameString
  Status = AmlOPNameString (Name, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Start %a NameString object\n", __FUNCTION__, Name));
    goto Done;
  }

  Status = InternalAmlLocateObjectByIdentifier (&Object, "EXTERNAL", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, Name));
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
    DEBUG ((DEBUG_ERROR, "%a: ERROR: %a has no child data.\n", __FUNCTION__, Name));
    goto Done;
  }

  Object->Data = AllocateZeroPool (ChildObject->DataSize + 3);
  // AML_EXTERNAL_OP + Name + ObjectType + ArgumentCount
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, Name));
    goto Done;
  }

  Object->DataSize = 0;
  Object->Data[0]  = AML_EXTERNAL_OP;
  Object->DataSize++;
  CopyMem (
    &Object->Data[Object->DataSize],
    ChildObject->Data,
    ChildObject->DataSize
    );
  Object->DataSize              += ChildObject->DataSize;
  Object->Data[Object->DataSize] = ObjectType;
  Object->DataSize++;
  Object->Data[Object->DataSize] = NumArgs;
  Object->DataSize++;

  InternalFreeAmlObject (&ChildObject, ListHead);
  Object->Completed = TRUE;
  Status            = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Locates the AML object that holds the cumulative offset term.
  This is the node directly after the node designated by
  GENERIC_FIELD_IDENTIFIER in Object->Data.

  @param[out]     ReturnObject  - Object that contains the offset term
  @param[in,out]  ListHead      - Linked list that contains the GENERIC_FIELD_IDENTIFIER

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlLocateOffsetTerm (
  OUT     AML_OBJECT_INSTANCE  **ReturnObject,
  IN OUT  LIST_ENTRY           *ListHead
  )
{
  LIST_ENTRY           *Node;
  AML_OBJECT_INSTANCE  *Object;
  UINTN                IdentifierSize;
  CHAR8                *Identifier;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Object         = NULL;
  *ReturnObject  = NULL;
  Identifier     = GENERIC_FIELD_IDENTIFIER;
  IdentifierSize = AsciiStrLen (Identifier) + 1;
  // Look Backwards and find Node for this Object
  Node = ListHead;
  do {
    Node   = GetPreviousNode (ListHead, Node);
    Object = AML_OBJECT_INSTANCE_FROM_LINK (Node);
    if ((Object->DataSize != 0) &&
        (Object->DataSize == IdentifierSize) &&
        (CompareMem (
           Object->Data,
           Identifier,
           MAX (Object->DataSize, IdentifierSize)
           ) == 0))
    {
      break;
    }
  } while (Node != ListHead);

  // Check to make sure FIELD is found, otherwise error
  if ((Object->DataSize == 0) ||
      (Object->DataSize != IdentifierSize) ||
      CompareMem (
        Object->Data,
        Identifier,
        (MAX (Object->DataSize, IdentifierSize) != 0)
        ))
  {
    return EFI_DEVICE_ERROR;
  }

  // Have found FIELD
  Node          = GetNextNode (ListHead, Node);
  Object        = AML_OBJECT_INSTANCE_FROM_LINK (Node);
  *ReturnObject = Object;
  return EFI_SUCCESS;
}

/**
  Offset (ByteOffset)

  Creates a ReservedField if the passed ByteOffset is larger than
  the previous bit length value optionally specified by an AmlOPFieldListItem,
  or another Offset call. All offsets are defined starting from zero, based at
  the starting address of the parent Operation Region.

  ReservedField := 0x00 PkgLength

  @param[in]      ByteLength    -Byte offset of the next defined field within
                                 the parent Operation Region
  @param[in,out]  ListHead      - Linked list has completed Offset object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPOffset (
  IN      UINT32      ByteOffset,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *OffsetObject;
  UINT8                *PkgLength;
  UINTN                DataLength;
  EFI_STATUS           Status;
  UINT64               InternalOffsetData;
  UINT64               BitCount;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InternalOffsetData = 0;
  BitCount           = LShiftU64 (ByteOffset, 3);
  Object             = NULL;
  OffsetObject       = NULL;
  PkgLength          = NULL;

  // Find and read internal offset data
  Status = InternalAmlLocateOffsetTerm (&OffsetObject, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Locating internal offset term\n", __FUNCTION__));
    goto Done;
  }

  InternalOffsetData = *(UINT64 *)OffsetObject->Data;

  if (InternalOffsetData > BitCount) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Invalid backwards offset\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  } else if (InternalOffsetData == BitCount) {
    // Do not need to append any reserved fields
    Status = EFI_SUCCESS;
    goto Done;
  }

  // update internal offset value to new offset
  *(UINT64 *)OffsetObject->Data = BitCount;

  // take difference to find how many bits to reserve
  BitCount = BitCount - InternalOffsetData;

  // Create new object for the offset data, add pkglength encoding
  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: appending new AML object\n", __FUNCTION__));
    goto Done;
  }

  Status = InternalAmlBitPkgLength ((UINT32)BitCount, &PkgLength, &DataLength);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: internal AML PkgLength\n", __FUNCTION__));
    goto Done;
  }

  Object->DataSize = DataLength + 1; // add one for Reserved Field Indicator
  Object->Data     = AllocateZeroPool (Object->DataSize);

  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for Offset\n", __FUNCTION__));
    goto Done;
  }

  Object->Data[0] = 0;
  CopyMem (&Object->Data[1], PkgLength, DataLength); // read internal offset data
  Object->Completed = TRUE;
  FreePool (PkgLength);

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    InternalFreeAmlObject (&OffsetObject, ListHead);
  }

  return Status;
}

/**
  Creates a NamedField or a ReservedField, depending on the
  parameters.

  To create a NamedField item pass in the NameSeg and Bitlength
  as in ASL. To create a ReservedField pass "" as the Name.
  Must be used inside a Field or IndexField TermList.

  NamedField := NameSeg PkgLength
  ReservedField := 0x00 PkgLength

  @param[in]      Name          - Field NameSeg
  @param[in]      BitLength     - Length of field item in bits
  @param[in,out]  ListHead      - Linked list has completed FieldUnitItem

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPFieldUnit (
  IN      CHAR8       *Name,
  IN      UINT32      BitLength,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *OffsetObject;
  EFI_STATUS           Status;

  if ((ListHead == NULL) || (Name == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_DEVICE_ERROR;
  Object       = NULL;
  OffsetObject = NULL;

  if (AsciiStrLen (Name) == 0) {
    if (BitLength > 0) {
      // Prepend a 0 to the list
      Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Unable to append AML object.\n", __FUNCTION__));
        goto Done;
      }

      Object->Data      = AllocateZeroPool (1);
      Object->DataSize  = 1;
      Object->Completed = TRUE;
    } else {
      Status = EFI_SUCCESS;
      goto Done;
    }
  } else {
    // add NameSeg to List
    Status = InternalAmlNameSeg (Name, ListHead);
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // Locate and update internal Offset term
  Status = InternalAmlLocateOffsetTerm (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Locating offset term for %a object\n", __FUNCTION__, Name));
    goto Done;
  }

  *(UINT64 *)Object->Data += BitLength; // write

  // Add BitLength as a PkgLength term
  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Appending BitLength for %a object\n", __FUNCTION__, Name));
    goto Done;
  }

  Status = InternalAmlBitPkgLength (BitLength, &Object->Data, &Object->DataSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Appending BitLength for %a object\n", __FUNCTION__, Name));
    goto Done;
  }

  Object->Completed = TRUE;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    InternalFreeAmlObject (&OffsetObject, ListHead);
  }

  return Status;
}

/**
  Creates a Field

  Field (RegionName, AccessType, LockRule, UpdateRule) {FieldUnitList}

  FieldUnitList must be added between AmlStart and AmlClose phase

  DefField := FieldOp PkgLength NameString FieldFlags FieldList
  FieldOp := ExtOpPrefix 0x81

  @param[in]      Phase         - Either AmlStart or AmlClose
  @param[in]      Name          - Field NameString
  @param[in]      AccessType    - Access Type for field
  @param[in]      LockRule      - Lock rule for field
  @param[in]      UpdateRule    - Update rule for field
  @param[in,out]  ListHead      - Linked list has completed Field Object after
                                  AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlField (
  IN      AML_FUNCTION_PHASE                   Phase,
  IN      CHAR8                                *Name,
  IN      EFI_ACPI_FIELD_ACCESS_TYPE_KEYWORDS  AccessType,
  IN      EFI_ACPI_FIELD_LOCK_RULE_KEYWORDS    LockRule,
  IN      EFI_ACPI_FIELD_UPDATE_RULE_KEYWORDS  UpdateRule,
  IN OUT  LIST_ENTRY                           *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINT8                FieldFlags;
  UINTN                ChildCount;

  if ((ListHead == NULL) || (Name == NULL) || (AsciiStrLen (Name) == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // parameter validation
  if ((Phase > AmlClose) || (AccessType > BufferAcc) || (LockRule > Lock) || (UpdateRule > WriteAsZeros)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, GENERIC_FIELD_IDENTIFIER, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start Field for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Insert internal offset counter
      Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Unable to append AML object.\n", __FUNCTION__));
        goto Done;
      }

      Object->DataSize  = sizeof (UINT64);
      Object->Data      = AllocateZeroPool (Object->DataSize);
      Object->Completed = TRUE;
      if (EFI_ERROR (Status) || (Object->Data == NULL)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start Field internal offset %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start PkgLength for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Insert required NameString
      Status = AmlOPNameString (Name, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Add Field Flags
      Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start FIELD_FLAGS for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Field Flags is one byte
      Object->DataSize = 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, Name));
        goto Done;
      }

      FieldFlags = (UINT8)((UpdateRule << 5) | (LockRule << 4) | AccessType);

      Object->Data[0]   = FieldFlags;
      Object->Completed = TRUE;

      // TermList is too complicated and must be added outside
      break;

    case AmlClose:

      // Required NameString completed in one phase call

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Close PkgLength for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Remove internal offset counter
      Status = InternalAmlLocateOffsetTerm (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locating internal offset term%a\n", __FUNCTION__, Name));
        goto Done;
      }

      Status = InternalFreeAmlObject (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: removing offset term%a\n", __FUNCTION__, Name));
        goto Done;
      }

      // remove original field identifier data
      Status = InternalAmlLocateObjectByIdentifier (&Object, GENERIC_FIELD_IDENTIFIER, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, Name));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, Name));
        goto Done;
      }

      // Field Op is two bytes
      Object->DataSize = ChildObject->DataSize + 2;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, Name));
        goto Done;
      }

      Object->Data[0] = AML_EXT_OP;
      Object->Data[1] = AML_EXT_FIELD_OP;
      CopyMem (
        &Object->Data[2],
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates a BankField

  BankField (RegionName, BankName, BankValue, AccessType, LockRule, UpdateRule) {FieldUnitList}
  FieldUnitList must be added between AmlStart and AmlClose phase

  DefBankField  := BankFieldOp PkgLength NameString NameString BankValue FieldFlags FieldList
  BankFieldOp   := ExtOpPrefix 0x87
  BankValue     := TermArg => Integer

  @param[in]      Phase         - Either AmlStart or AmlClose
  @param[in]      RegionName    - Name of host Operation Region
  @param[in]      BankName      - Name of bank selection register
  @param[in]      BankValue     - Bank Selection ID
  @param[in]      AccessType    - Access Type as in Field
  @param[in]      LockRule      - Lock rule as in Field
  @param[in]      UpdateRule    - Update rule as in Field
  @param[in,out]  ListHead      - Linked list has completed BankField Object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlBankField (
  IN      AML_FUNCTION_PHASE                   Phase,
  IN      CHAR8                                *RegionName,
  IN      CHAR8                                *BankName,
  IN      UINT64                               BankValue,
  IN      EFI_ACPI_FIELD_ACCESS_TYPE_KEYWORDS  AccessType,
  IN      EFI_ACPI_FIELD_LOCK_RULE_KEYWORDS    LockRule,
  IN      EFI_ACPI_FIELD_UPDATE_RULE_KEYWORDS  UpdateRule,
  IN OUT  LIST_ENTRY                           *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINT8                FieldFlags;
  UINTN                ChildCount;

  if ((ListHead == NULL) || (RegionName == NULL) || (AsciiStrLen (RegionName) == 0) ||
      (BankName == NULL) || (AsciiStrLen (BankName) == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // parameter validation
  if ((Phase > AmlClose) || (AccessType > BufferAcc) || (LockRule > Lock) || (UpdateRule > WriteAsZeros)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, GENERIC_FIELD_IDENTIFIER, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start BankField for %a object\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Insert internal offset counter
      Status            = InternalAppendNewAmlObjectNoData (&Object, ListHead);
      Object->DataSize  = sizeof (UINT64);
      Object->Data      = AllocateZeroPool (Object->DataSize);
      Object->Completed = TRUE;
      if (EFI_ERROR (Status) || (Object->Data == NULL)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start BankField internal offset %a object\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start PkgLength for %a object\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Insert required Region NameString
      Status = AmlOPNameString (RegionName, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString for %a object\n", __FUNCTION__, RegionName));
        goto Done;
      }

      // Insert required Bank NameString
      Status = AmlOPNameString (BankName, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString for %a object\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Insert required BankValue integer
      Status = AmlOPDataInteger (BankValue, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Adding BankValue Integer for %a object\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Add Field Flags
      Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start FIELD_FLAGS for %a object\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Field Flags is one byte
      Object->DataSize = 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, BankName));
        goto Done;
      }

      FieldFlags = (UINT8)((UpdateRule << 5) | (LockRule << 4) | AccessType);

      Object->Data[0]   = FieldFlags;
      Object->Completed = TRUE;

      // TermList is too complicated and must be added outside
      break;

    case AmlClose:

      // Required NameStrings completed in one phase call

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Close PkgLength for %a object\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Remove internal offset counter
      Status = InternalAmlLocateOffsetTerm (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locating internal offset term%a\n", __FUNCTION__, BankName));
        goto Done;
      }

      Status = InternalFreeAmlObject (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: removing offset term%a\n", __FUNCTION__, BankName));
        goto Done;
      }

      // remove original field identifier data
      Status = InternalAmlLocateObjectByIdentifier (&Object, GENERIC_FIELD_IDENTIFIER, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, BankName));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, BankName));
        goto Done;
      }

      // Field Op is two bytes
      Object->DataSize = ChildObject->DataSize + 2;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, BankName));
        goto Done;
      }

      Object->Data[0] = AML_EXT_OP;
      Object->Data[1] = AML_EXT_BANK_FIELD_OP;
      CopyMem (
        &Object->Data[2],
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates an IndexField

  IndexField (IndexName, DataName, AccessType, LockRule, UpdateRule) {FieldUnitList}

  FieldUnitList must be added between AmlStart and AmlClose phase

  DefIndexField := IndexFieldOp PkgLength NameString NameString FieldFlags FieldList
  IndexFieldOp  := ExtOpPrefix 0x86

  @param[in]      Phase         - Either AmlStart or AmlClose
  @param[in]      IndexName     - Name of Index FieldUnit
  @param[in]      DataName      - Name of Data FieldUnit
  @param[in]      AccessType    - Access Type for the FieldUnit
  @param[in]      LockRule      - Lock rule for the FieldUnit
  @param[in]      UpdateRule    - Update rule for the FieldUnit
  @param[in,out]  ListHead      - Linked list has completed IndexField Object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlIndexField (
  IN      AML_FUNCTION_PHASE                   Phase,
  IN      CHAR8                                *IndexName,
  IN      CHAR8                                *DataName,
  IN      EFI_ACPI_FIELD_ACCESS_TYPE_KEYWORDS  AccessType,
  IN      EFI_ACPI_FIELD_LOCK_RULE_KEYWORDS    LockRule,
  IN      EFI_ACPI_FIELD_UPDATE_RULE_KEYWORDS  UpdateRule,
  IN OUT  LIST_ENTRY                           *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINT8                FieldFlags;
  UINTN                ChildCount;

  if ((ListHead == NULL) || (IndexName == NULL) || (AsciiStrLen (IndexName) == 0) ||
      (DataName == NULL) || (AsciiStrLen (DataName) == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // parameter validation
  if ((Phase > AmlClose) || (AccessType > BufferAcc) || (LockRule > Lock) || (UpdateRule > WriteAsZeros)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, GENERIC_FIELD_IDENTIFIER, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start IndexField for %a object\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // Insert internal offset counter
      Status            = InternalAppendNewAmlObjectNoData (&Object, ListHead);
      Object->DataSize  = sizeof (UINT64);
      Object->Data      = AllocateZeroPool (Object->DataSize);
      Object->Completed = TRUE;
      if (EFI_ERROR (Status) || (Object->Data == NULL)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start IndexField internal offset %a object\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start PkgLength for %a object\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // Insert required Index NameString
      Status = AmlOPNameString (IndexName, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString for %a object\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // Insert required Data NameString
      Status = AmlOPNameString (DataName, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString for %a object\n", __FUNCTION__, DataName));
        goto Done;
      }

      // Add Field Flags
      Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start FIELD_FLAGS for %a object\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // Field Flags is one byte
      Object->DataSize = 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, IndexName));
        goto Done;
      }

      FieldFlags = (UINT8)((UpdateRule << 5) | (LockRule << 4) | AccessType);

      Object->Data[0]   = FieldFlags;
      Object->Completed = TRUE;

      // TermList is too complicated and must be added outside
      break;

    case AmlClose:

      // Required NameString completed in one phase call

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Close PkgLength for %a object\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // Remove internal offset counter
      Status = InternalAmlLocateOffsetTerm (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locating internal offset term%a\n", __FUNCTION__, IndexName));
        goto Done;
      }

      Status = InternalFreeAmlObject (&Object, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: removing offset term%a\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // remove original field identifier data
      Status = InternalAmlLocateObjectByIdentifier (&Object, GENERIC_FIELD_IDENTIFIER, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, IndexName));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, IndexName));
        goto Done;
      }

      // Field Op is two bytes
      Object->DataSize = ChildObject->DataSize + 2;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, IndexName));
        goto Done;
      }

      Object->Data[0] = AML_EXT_OP;
      Object->Data[1] = AML_EXT_INDEX_FIELD_OP;
      CopyMem (
        &Object->Data[2],
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  OperationRegion (RegionName, RegionSpace, Offset, Length)

  DefOpRegion := OpRegionOp NameString RegionSpace RegionOffset RegionLen
  OpRegionOp := ExtOpPrefix 0x80
  RegionSpace :=
    ByteData // 0x00 SystemMemory
    // 0x01 SystemIO
    // 0x02 PCI_Config
    // 0x03 EmbeddedControl
    // 0x04 SMBus
    // 0x05 System CMOS
    // 0x06 PciBarTarget
    // 0x07 IPMI
    // 0x08 GeneralPurposeIO
    // 0x09 GenericSerialBus
    // 0x0A PCC
    // 0x80-0xFF: OEM Defined
  RegionOffset := TermArg => Integer
  RegionLen := TermArg => Integer

  @param[in]      RegionName   - Name for the Operation Region
  @param[in]      RegionSpace  - Region Space type
  @param[in]      Offset       - Offset within the selected RegionSpace at which the
                                 region starts (byte-granular)
  @param[in]      Length        - Length of the region in bytes.
  @param[in,out]  ListHead      - Linked list head

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPOperationRegion (
  IN      CHAR8                     *RegionName,
  IN      GENERIC_ADDRESS_SPACE_ID  RegionSpace,
  IN      UINT64                    Offset,
  IN      UINT64                    Length,
  IN OUT  LIST_ENTRY                *ListHead
  )
{
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;
  EFI_STATUS           Status;

  // Input parameter validation
  if ((RegionName == NULL) || (AsciiStrLen (RegionName) == 0) || (ListHead == NULL) ||
      ((RegionSpace > PCC) && (RegionSpace < 0x80)) || (RegionSpace > 0xFF))
  {
    return EFI_INVALID_PARAMETER;
  }

  Object      = NULL;
  ChildObject = NULL;
  Status      = EFI_DEVICE_ERROR;

  Status = InternalAppendNewAmlObject (&Object, "OPREGION", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Start OpRegion for %a object\n", __FUNCTION__, RegionName));
    goto Done;
  }

  Status = AmlOPNameString (RegionName, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Name String for %a object\n", __FUNCTION__, RegionName));
    goto Done;
  }

  Status = AmlOPByteData ((UINT8)RegionSpace, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Region space byte data for %a object\n", __FUNCTION__, RegionName));
    goto Done;
  }

  Status = AmlOPDataInteger (Offset, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Offset data integer for %a object\n", __FUNCTION__, RegionName));
    goto Done;
  }

  Status = AmlOPDataInteger (Length, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Length data integer for %a object\n", __FUNCTION__, RegionName));
    goto Done;
  }

  Status = InternalAmlLocateObjectByIdentifier (&Object, "OPREGION", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, RegionName));
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
    DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, RegionName));
    goto Done;
  }

  // OpRegion Opcode is two bytes
  Object->DataSize = ChildObject->DataSize + 2;
  Object->Data     = AllocatePool (Object->DataSize);
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, RegionName));
    goto Done;
  }

  Object->Data[0] = AML_EXT_OP;
  Object->Data[1] = AML_EXT_REGION_OP;
  CopyMem (
    &Object->Data[2],
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

/**
  Creates a CreateField AML Object and inserts it into the linked list

  Syntax:
  CreateField ( SourceBuffer, BitIndex, NumBits, FieldName )

  DefCreateField := CreateFieldOp SourceBuff BitIndex NumBits NameString
  CreateFieldOp  := ExtOpPrefix 0x13
  ExtOpPrefix    := 0x5B
  SourceBuff     := TermArg => Buffer
  BitIndex       := TermArg => Integer
  NumBits        := TermArg -> Integer

  @param[in]      SourceBuffer,   - Buffer to house the new buffer field object
  @param[in]      BitIndex,       - Starting bit index place the new buffer
  @param[in]      NumBits,        - Number of bits to reserve
  @param[in]      FieldName,      - The new buffer field object to be created in SourceBuffer
  @param[in,out]  ListHead        - Linked list has completed CreateField object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPCreateField (
  IN      CHAR8       *SourceBuffer,
  IN      UINT64      BitIndex,
  IN      UINT64      NumBits,
  IN      CHAR8       *FieldName,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  ChildObject = NULL;
  Object      = NULL;

  if ((SourceBuffer == NULL) || (FieldName == NULL) || (ListHead == NULL) ||
      (AsciiStrLen (SourceBuffer) == 0) || (AsciiStrLen (FieldName) == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  ChildObject = NULL;
  Status      = InternalAppendNewAmlObject (&Object, "CreateField", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: CreateField for %a object\n", __FUNCTION__, FieldName));
    goto Done;
  }

  Status = AmlOPNameString (SourceBuffer, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: NameString for %a object\n", __FUNCTION__, FieldName));
    goto Done;
  }

  Status = AmlOPDataInteger (BitIndex, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: BitIndex for %a object\n", __FUNCTION__, FieldName));
    goto Done;
  }

  Status = AmlOPDataInteger (NumBits, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: NumBits for %a object\n", __FUNCTION__, FieldName));
    goto Done;
  }

  Status = AmlOPNameString (FieldName, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: FieldName for %a object\n", __FUNCTION__, FieldName));
    goto Done;
  }

  Status = InternalAmlLocateObjectByIdentifier (&Object, "CreateField", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, FieldName));
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
    DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, FieldName));
    goto Done;
  }

  // CreateFieldOp is two bytes
  Object->DataSize = ChildObject->DataSize + 2;
  Object->Data     = AllocatePool (Object->DataSize);
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, FieldName));
    goto Done;
  }

  Object->Data[0] = AML_EXT_OP;
  Object->Data[1] = AML_EXT_CREATE_FIELD_OP;
  CopyMem (
    &Object->Data[2],
    ChildObject->Data,
    ChildObject->DataSize
    );
  InternalFreeAmlObject (&ChildObject, ListHead);
  Object->Completed = TRUE;

  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Internal function used to create a CreateBit|Byte|Word|DWord|QWordField objects
  and insert them into the linked list

  @param[in]      SourceBuffer,     - Buffer to insert the new buffer fixed field object
  @param[in]      Index,            - Starting index to place the new buffer
  @param[in]      FixedFieldName,   - Name of the FixedField
  @param[in]      OpCode,           - AML opcode for the Create_Field encoding
  @param[in,out]  ListHead          - Linked list has completed CreateFixedField object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlCreateFixedField (
  IN      CHAR8       *SourceBuffer,
  IN      UINT64      Index,
  IN      CHAR8       *FixedFieldName,
  IN      UINT8       OpCode,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;

  ChildObject = NULL;

  if ((SourceBuffer == NULL) || (FixedFieldName == NULL) || (ListHead == NULL) ||
      (AsciiStrLen (SourceBuffer) == 0) || (AsciiStrLen (FixedFieldName) == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = InternalAppendNewAmlObject (&Object, "CreateFixedField", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: CreateField for %a object\n", __FUNCTION__, FixedFieldName));
    goto Done;
  }

  // Check if Localx Buffer
  if (AsciiStrnCmp (SourceBuffer, "Local", 5) == 0) {
    if ((SourceBuffer[5] >= '0') && (SourceBuffer[5] <= '9')) {
      Status = AmlOPLocalN ((UINT8)AsciiStrDecimalToUintn (&SourceBuffer[5]), ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: LocalN for %a object\n", __FUNCTION__, FixedFieldName));
        goto Done;
      }
    }

    // Check if Argx Buffer
  } else if (AsciiStrnCmp (SourceBuffer, "Arg", 3) == 0) {
    if ((SourceBuffer[3] >= '0') && (SourceBuffer[3] <= '9')) {
      Status = AmlOpArgN ((UINT8)AsciiStrDecimalToUintn (&SourceBuffer[3]), ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: ArgN for %a object\n", __FUNCTION__, FixedFieldName));
        goto Done;
      }
    }
  } else {
    Status = AmlOPNameString (SourceBuffer, ListHead);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: ERROR: NameString for %a object\n", __FUNCTION__, FixedFieldName));
      goto Done;
    }
  }

  Status = AmlOPDataInteger (Index, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Index for %a object\n", __FUNCTION__, FixedFieldName));
    goto Done;
  }

  Status = AmlOPNameString (FixedFieldName, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: FieldName for %a object\n", __FUNCTION__, FixedFieldName));
    goto Done;
  }

  Status = InternalAmlLocateObjectByIdentifier (&Object, "CreateFixedField", ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, FixedFieldName));
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
    DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, FixedFieldName));
    goto Done;
  }

  // CreateWordFieldOp is one byte
  Object->DataSize = ChildObject->DataSize + 1;
  Object->Data     = AllocatePool (Object->DataSize);
  if (Object->Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, FixedFieldName));
    goto Done;
  }

  Object->Data[0] = OpCode;
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}

/**
  Creates a CreateBitField AML Object and inserts it into the linked list

  Syntax:
  CreateBitField (SourceBuffer, BitIndex, BitFieldName)

  DefCreateBitField   := CreateBitFieldOp SourceBuff BitIndex NameString
  CreateBitFieldOp    := 0x8D
  SourceBuff          := TermArg => Buffer
  BitIndex            := TermArg => Integer

  @param[in]      SourceBuffer,   - Buffer to insert the new buffer bit field object
  @param[in]      BitIndex,       - Starting bit index to place the new buffer
  @param[in]      BitFieldName,   - Name of the BitField
  @param[in,out]  ListHead        - Linked list has completed CreateBitField object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPCreateBitField (
  IN      CHAR8       *SourceBuffer,
  IN      UINT64      BitIndex,
  IN      CHAR8       *BitFieldName,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlCreateFixedField (SourceBuffer, BitIndex, BitFieldName, AML_CREATE_BIT_FIELD_OP, ListHead);
  return Status;
}

/**
  Creates a CreateByteField AML Object and inserts it into the linked list

  Syntax:
  CreateByteField ( SourceBuffer, ByteIndex, ByteFieldName )

  DefCreateByteField  := CreateByteFieldOp SourceBuff ByteIndex NameString
  CreateByteFieldOp   := 0x8C
  SourceBuff          := TermArg => Buffer
  ByteIndex           := TermArg => Integer

  @param[in]      SourceBuffer,   - Buffer to insert the new buffer byte field object
  @param[in]      ByteIndex,      - Starting byte index to place the new buffer
  @param[in]      ByteFieldName,  - Name of the ByteField
  @param[in,out]  ListHead        - Linked list has completed CreateByteField object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPCreateByteField (
  IN      CHAR8       *SourceBuffer,
  IN      UINT64      ByteIndex,
  IN      CHAR8       *ByteFieldName,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlCreateFixedField (SourceBuffer, ByteIndex, ByteFieldName, AML_CREATE_BYTE_FIELD_OP, ListHead);
  return Status;
}

/**
  Creates a CreateDWordField AML Object and inserts it into the linked list

  Syntax:
  CreateDWordField ( SourceBuffer, ByteIndex, DWordFieldName )

  DefCreateDWordField := CreateDWordFieldOp SourceBuff ByteIndex NameString
  CreateDWordFieldOp  := 0x8A
  SourceBuff          := TermArg => Buffer
  ByteIndex           := TermArg => Integer

  @param[in]      SourceBuffer,     - Buffer to insert the new buffer DWord field object
  @param[in]      ByteIndex,        - Starting byte index to place the new buffer
  @param[in]      DWordFieldName,   - Name of the DWordField
  @param[in,out]  ListHead          - Linked list has completed CreateDWordField object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPCreateDWordField (
  IN      CHAR8       *SourceBuffer,
  IN      UINT64      ByteIndex,
  IN      CHAR8       *DWordFieldName,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlCreateFixedField (SourceBuffer, ByteIndex, DWordFieldName, AML_CREATE_DWORD_FIELD_OP, ListHead);
  return Status;
}

/**
  Creates a CreateQWordField AML Object and inserts it into the linked list

  Syntax:
  CreateQWordField ( SourceBuffer, ByteIndex, QWordFieldName )

  DefCreateQWordField := CreateQWordFieldOp SourceBuff ByteIndex NameString
  CreateQWordFieldOp  := 0x8F
  SourceBuff          := TermArg => Buffer
  ByteIndex           := TermArg => Integer

  @param[in]      SourceBuffer,     - Buffer to insert the new buffer QWord field object
  @param[in]      ByteIndex,        - Starting byte index to place the new buffer
  @param[in]      QWordFieldName,   - Name of the QWordField
  @param[in,out]  ListHead          - Linked list has completed CreateQWordField object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPCreateQWordField (
  IN      CHAR8       *SourceBuffer,
  IN      UINT64      ByteIndex,
  IN      CHAR8       *QWordFieldName,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlCreateFixedField (SourceBuffer, ByteIndex, QWordFieldName, AML_CREATE_QWORD_FIELD_OP, ListHead);
  return Status;
}

/**
  Creates a CreateWordField AML Object and inserts it into the linked list

  Syntax:
  CreateWordField ( SourceBuffer, ByteIndex, WordFieldName )

  DefCreateWordField  := CreateWordFieldOp SourceBuff ByteIndex NameString
  CreateWordFieldOp   := 0x8B
  SourceBuff          := TermArg => Buffer
  ByteIndex           := TermArg => Integer

  @param[in]      SourceBuffer,   - Buffer to house the new buffer word field object
  @param[in]      ByteIndex,      - Starting byte index to place the new buffer
  @param[in]      WordFieldName,  - Name of the WordField
  @param[in,out]  ListHead        - Linked list has completed CreateWordField object

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlOPCreateWordField (
  IN      CHAR8       *SourceBuffer,
  IN      UINT64      ByteIndex,
  IN      CHAR8       *WordFieldName,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS  Status;

  Status = InternalAmlCreateFixedField (SourceBuffer, ByteIndex, WordFieldName, AML_CREATE_WORD_FIELD_OP, ListHead);
  return Status;
}

/**
  Creates a Method

  Method (MethodName, NumArgs, SerializeRule, SyncLevel, ReturnType,
          ParameterTypes) {TermList}

  TermList must be created between AmlStart and AmlClose Phase

  Note: ReturnType and ParameterTypes are not used for AML encoding
        and are therefore not passed in to this function.

  DefMethod    := MethodOp PkgLength NameString MethodFlags TermList
  MethodOp     := 0x14
  MethodFlags  := ByteData  // bit 0-2: ArgCount (0-7)
                            // bit 3: SerializeFlag
                            // 0 NotSerialized
                            // 1 Serialized
                            // bit 4-7: SyncLevel (0x00-0x0f)

  @param[in]      Phase         - Either AmlStart or AmlClose
  @param[in]      Name          - Method name
  @param[in]      NumArgs       - Number of arguments passed in to method
  @param[in]      SerializeRule - Flag indicating whether method is serialized
                                  or not
  @param[in]      SyncLevel     - synchronization level for the method (0 - 15),
                                  use zero for default sync level.
  @param[in,out]  ListHead      - Linked list has completed String Object after
                                  AmlClose.

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
AmlMethod (
  IN      AML_FUNCTION_PHASE     Phase,
  IN      CHAR8                  *Name,
  IN      UINT8                  NumArgs,
  IN      METHOD_SERIALIZE_FLAG  SerializeRule,
  IN      UINT8                  SyncLevel,
  IN OUT  LIST_ENTRY             *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINT8                MethodFlags;
  UINTN                ChildCount;

  if ((Phase >= AmlInvalid) ||
      (Name == NULL) ||
      (NumArgs > METHOD_ARGS_MAX) ||
      (SyncLevel > MAX_SYNC_LEVEL) ||
      (SerializeRule >= FlagInvalid) ||
      (ListHead == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, "Method", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start Method for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Start required PkgLength
      Status = AmlPkgLength (AmlStart, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start PkgLength for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Insert required NameString
      Status = AmlOPNameString (Name, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // Add Method Flags
      Status = InternalAppendNewAmlObject (&Object, "METHOD_FLAGS", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start METHOD_FLAGS for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      // TermList is too complicated and must be added outside
      break;

    case AmlClose:
      // TermList should be closed already
      // Add Method Flags
      Status = InternalAmlLocateObjectByIdentifier (&Object, "METHOD_FLAGS", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locate METHOD_FLAGS for %a object\n", __FUNCTION__, Name));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a METHOD_FLAGS child data collection.\n", __FUNCTION__, Name));
        goto Done;
      }

      // Method Flags is one byte
      Object->DataSize = ChildObject->DataSize + 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, Name));
        goto Done;
      }

      MethodFlags = NumArgs & 0x07;
      if (SerializeRule) {
        MethodFlags |= BIT3;
      }

      MethodFlags    |= (SyncLevel & 0x0F) << 4;
      Object->Data[0] = MethodFlags;
      CopyMem (
        &Object->Data[1],
        ChildObject->Data,
        ChildObject->DataSize
        );
      InternalFreeAmlObject (&ChildObject, ListHead);
      Object->Completed = TRUE;

      // Required NameString completed in one phase call

      // Close required PkgLength before finishing Object
      Status = AmlPkgLength (AmlClose, ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Close PkgLength for %a object\n", __FUNCTION__, Name));
        goto Done;
      }

      Status = InternalAmlLocateObjectByIdentifier (&Object, "Method", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Locate %a object\n", __FUNCTION__, Name));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a child data collection.\n", __FUNCTION__, Name));
        goto Done;
      }

      // Method Op is one byte
      Object->DataSize = ChildObject->DataSize + 1;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocate Object->Data for %a\n", __FUNCTION__, Name));
        goto Done;
      }

      Object->Data[0] = AML_METHOD_OP;
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
    InternalFreeAmlObject (&ChildObject, ListHead);
    InternalFreeAmlObject (&Object, ListHead);
  }

  return Status;
}
