/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"

#define MAX_ONE_BYTE_PKG_LENGTH       63
#define ONE_BYTE_PKG_LENGTH_ENCODING  0x00
#define ONE_BYTE_NIBBLE_MASK          0x3F

#define MAX_TWO_BYTE_PKG_LENGTH       4095
#define TWO_BYTE_PKG_LENGTH_ENCODING  0x40
#define PKG_LENGTH_NIBBLE_MASK        0x0F

#define MAX_THREE_BYTE_PKG_LENGTH       1048575
#define THREE_BYTE_PKG_LENGTH_ENCODING  0x80

#define MAX_FOUR_BYTE_PKG_LENGTH       268435455
#define FOUR_BYTE_PKG_LENGTH_ENCODING  0xC0

/**
  Creates a Package Length encoding and places it in the return buffer,
  PkgLengthEncoding. Similar to AmlPkgLength but the PkgLength does not
  include the length of its own encoding.

  @param[in]   DataSize  - The size of data to be encoded as a pkglength
  @param[out]  PkgLengthEncoding  - Return buffer containing the AML encoding
  @param[out]  ReturnDataLength  - Size of the return buffer

  @return   EFI_SUCCESS     - Success
  @return   all others      - Fail
  **/
EFI_STATUS
EFIAPI
InternalAmlBitPkgLength (
  IN   UINT32  DataSize,
  OUT  UINT8   **PkgLengthEncoding,
  OUT  UINTN   *ReturnDataLength
  )
{
  UINTN       DataLength;
  UINT8       PkgLeadByte;
  UINTN       PkgLengthRemainder;
  EFI_STATUS  Status;

  Status     = EFI_INVALID_PARAMETER;
  DataLength = 0;
  // Calculate Length of PkgLength Data and fill out least
  // significant nibble
  if ((DataSize + 1) <= MAX_ONE_BYTE_PKG_LENGTH) {
    DataLength   = 1;
    PkgLeadByte  = ONE_BYTE_PKG_LENGTH_ENCODING;
    PkgLeadByte |= ((DataSize) & ONE_BYTE_NIBBLE_MASK);
  } else {
    if ((DataSize + 2) <= MAX_TWO_BYTE_PKG_LENGTH) {
      DataLength  = 2;
      PkgLeadByte = TWO_BYTE_PKG_LENGTH_ENCODING;
    } else if ((DataSize + 3) <= MAX_THREE_BYTE_PKG_LENGTH) {
      DataLength  = 3;
      PkgLeadByte = THREE_BYTE_PKG_LENGTH_ENCODING;
    } else if ((DataSize + 4) <= MAX_FOUR_BYTE_PKG_LENGTH) {
      DataLength  = 4;
      PkgLeadByte = FOUR_BYTE_PKG_LENGTH_ENCODING;
    } else {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "%a: ERROR: PkgLength data size > 0x%X\n",
        __FUNCTION__,
        MAX_FOUR_BYTE_PKG_LENGTH - 4
        ));
      goto Done;
    }

    PkgLeadByte |= ((DataSize) & PKG_LENGTH_NIBBLE_MASK);
  }

  // Allocate new data buffer
  // DataSize = DataLength + DataSize;
  *PkgLengthEncoding = AllocatePool (DataLength);
  if (*PkgLengthEncoding == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: allocation failed Object=PkgLength\n", __FUNCTION__));
    goto Done;
  }

  // Populate PkgLeadByte
  *PkgLengthEncoding[0] = PkgLeadByte;

  // Populate remainder of PkgLength bytes
  PkgLengthRemainder = (DataSize) >> 4;
  if (PkgLengthRemainder != 0) {
    CopyMem (&PkgLengthEncoding[0][1], &PkgLengthRemainder, DataLength - 1);
  }

  *ReturnDataLength = DataLength;

Done:
  return Status;
}

/**
  Creates a Package Length AML Object and inserts it into the linked list

  PkgLength := PkgLeadByte |
               <PkgLeadByte ByteData> |
               <PkgLeadByte ByteData ByteData> |
               <PkgLeadByte ByteData ByteData ByteData>

  PkgLeadByte := <bit 7-6: ByteData count that follows (0-3)>
                 <bit 5-4: Only used if PkgLength < 63>
                 <bit 3-0: Least significant package length nybble>

  Note: The high 2 bits of the first byte reveal how many follow bytes are in
  the PkgLength. If the PkgLength has only one byte, bit 0 through 5 are used
  to encode the package length (in other words, values 0-63). If the package
  length value is more than 63, more than one byte must be used for the encoding
  in which case bit 4 and 5 of the PkgLeadByte are reserved and must be zero.

  If the multiple bytes encoding is used, bits 0-3 of the PkgLeadByte become
  the least significant 4 bits of the resulting package length value. The next
  ByteData will become the next least significant 8 bits of the resulting value
  and so on, up to 3 ByteData bytes. Thus, the maximum package length is 2**28.

  @param[in]      Phase     - Example: AmlStart, AmlClose
  @param[in,out]  ListHead  - Head of Linked List of all AML Objects

  @return   EFI_SUCCESS     - Success
  @return   all others      - Fail
  **/
EFI_STATUS
EFIAPI
AmlPkgLength (
  IN      AML_FUNCTION_PHASE  Phase,
  IN OUT  LIST_ENTRY          *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  AML_OBJECT_INSTANCE  *ChildObject;
  UINTN                ChildCount;
  UINTN                DataLength;
  UINT8                PkgLeadByte;
  UINTN                PkgLengthRemainder;

  if ((Phase >= AmlInvalid) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_DEVICE_ERROR;
  Object      = NULL;
  ChildObject = NULL;

  switch (Phase) {
    case AmlStart:
      Status = InternalAppendNewAmlObject (&Object, "LENGTH", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: Start Length object\n", __FUNCTION__));
        goto Done;
      }

      break;
    case AmlClose:
      Status = InternalAmlLocateObjectByIdentifier (&Object, "LENGTH", ListHead);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: ERROR: locate Length object\n", __FUNCTION__));
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
        DEBUG ((DEBUG_ERROR, "%a: ERROR: %a has no child data.\n", __FUNCTION__, "Length"));
        goto Done;
      }

      DataLength = 0;
      // Calculate Length of PkgLength Data and fill out least
      // significant nibble
      if ((ChildObject->DataSize + 1) <= MAX_ONE_BYTE_PKG_LENGTH) {
        DataLength   = 1;
        PkgLeadByte  = ONE_BYTE_PKG_LENGTH_ENCODING;
        PkgLeadByte |= ((ChildObject->DataSize + DataLength) & ONE_BYTE_NIBBLE_MASK);
      } else {
        if ((ChildObject->DataSize + 2) <= MAX_TWO_BYTE_PKG_LENGTH) {
          DataLength  = 2;
          PkgLeadByte = TWO_BYTE_PKG_LENGTH_ENCODING;
        } else if ((ChildObject->DataSize + 3) <= MAX_THREE_BYTE_PKG_LENGTH) {
          DataLength  = 3;
          PkgLeadByte = THREE_BYTE_PKG_LENGTH_ENCODING;
        } else if ((ChildObject->DataSize + 4) <= MAX_FOUR_BYTE_PKG_LENGTH) {
          DataLength  = 4;
          PkgLeadByte = FOUR_BYTE_PKG_LENGTH_ENCODING;
        } else {
          Status = EFI_DEVICE_ERROR;
          DEBUG ((
            DEBUG_ERROR,
            "%a: ERROR: PkgLength data size > 0x%X\n",
            __FUNCTION__,
            MAX_FOUR_BYTE_PKG_LENGTH - 4
            ));
          goto Done;
        }

        PkgLeadByte |= ((ChildObject->DataSize + DataLength) & PKG_LENGTH_NIBBLE_MASK);
      }

      // Allocate new data buffer
      Object->DataSize = DataLength + ChildObject->DataSize;
      Object->Data     = AllocatePool (Object->DataSize);
      if (Object->Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: allocation failed Object=PkgLength\n", __FUNCTION__));
        goto Done;
      }

      // Populate PkgLeadByte
      Object->Data[0] = PkgLeadByte;

      // Populate remainder of PkgLength bytes
      PkgLengthRemainder = (ChildObject->DataSize + DataLength) >> 4;
      if (PkgLengthRemainder != 0) {
        CopyMem (&Object->Data[1], &PkgLengthRemainder, DataLength - 1);
      }

      CopyMem (
        &Object->Data[DataLength],
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
