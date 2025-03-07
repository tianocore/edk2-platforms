/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalAmlLib.h"

#define MAX_NAME_SEG_COUNT  255

/*
  Is character a RootChar

  @param[in]      TestChar  - Character to check

  @return   TRUE    - Character is a RootChar
  @return   FALSE   - Character is not a RootChar
  */
BOOLEAN
InternalIsRootChar (
  IN      CHAR8  TestChar
  )
{
  if (TestChar == AML_ROOT_CHAR) {
    return TRUE;
  }

  return FALSE;
}

/*
  Is character a ParentPrefixChar

  @param[in]      TestChar  - Character to check

  @return   TRUE    - Character is a ParentPrefixChar
  @return   FALSE   - Character is not a ParentPrefixChar
  */
BOOLEAN
InternalIsParentPrefixChar (
  IN      CHAR8  TestChar
  )
{
  if (TestChar == AML_PARENT_PREFIX_CHAR) {
    return TRUE;
  }

  return FALSE;
}

/*
  Is character a LeadNameChar = '_', 'A' - 'Z'

  @param[in]      TestChar  - Character to check

  @return   TRUE    - Character is a LeadNameChar
  @return   FALSE   - Character is not a LeadNameChar
  */
BOOLEAN
InternalIsLeadNameChar (
  IN      CHAR8  TestChar
  )
{
  if ( // Allowed LeadNameChars '_', 'A'-'Z'
       (TestChar == AML_NAME_CHAR__) ||
       ((TestChar >= AML_NAME_CHAR_A) &&
        (TestChar <= AML_NAME_CHAR_Z))
       )
  {
    return TRUE;
  }

  return FALSE;
}

/*
  Is character a DigitChar = '0' - '9'

  @param[in]      TestChar  - Character to check

  @return   TRUE    - Character is a DigitChar
  @return   FALSE   - Character is not a DigitChar
  */
BOOLEAN
InternalIsDigitChar (
  IN      CHAR8  TestChar
  )
{
  if ( // Allowed DigitChars '0'-'9'
       (TestChar >= AML_DIGIT_CHAR_0) &&
       (TestChar <= AML_DIGIT_CHAR_9)
       )
  {
    return TRUE;
  }

  return FALSE;
}

/*
  Is character a NameChar = LeadNameChar | DigitChar

  @param[in]      TestChar  - Character to check

  @return   TRUE    - Character is a NameChar
  @return   FALSE   - Character is not a NameChar
  */
BOOLEAN
InternalIsNameChar (
  IN      CHAR8  TestChar
  )
{
  if ( // Allowed LeadNameChar and DigitChars
       InternalIsDigitChar (TestChar) ||
       InternalIsLeadNameChar (TestChar)
       )
  {
    return TRUE;
  }

  return FALSE;
}

/*
  Is character a NameSeg separator

  @param[in]      TestChar  - Character to check

  @return   TRUE    - Character is a NameChar
  @return   FALSE   - Character is not a NameChar
  */
BOOLEAN
InternalIsNameSegSeparator (
  IN      CHAR8  TestChar
  )
{
  if (TestChar == '.') {
    return TRUE;
  }

  return FALSE;
}

/**
  Creates a NameSeg AML object and inserts it into the List

  NameSeg := <leadnamechar namechar namechar namechar>

  NameSegs shorter than 4 characters are filled with trailing underscores

  @param[in]      Name          - NameSeg
  @param[in,out]  ListHead      - Linked list has NameSeg after call

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlNameSeg (
  IN      CHAR8       *Name,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  AML_OBJECT_INSTANCE  *Object;
  UINT8                *NameSeg;
  UINTN                NameLen;
  EFI_STATUS           Status;

  if (Name == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NameLen = AsciiStrLen (Name);
  Status  = EFI_DEVICE_ERROR;
  Object  = NULL;
  NameSeg = NULL;
  // parameter validation
  if ((NameLen == 0) || (NameLen > 4)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!InternalIsLeadNameChar (Name[0])) {
    return EFI_INVALID_PARAMETER;
  }

  for (UINT8 i = 1; NameLen > 1 && i < NameLen; i++) {
    if (!InternalIsNameChar (Name[i])) {
      return EFI_INVALID_PARAMETER;
    }
  }

  NameSeg = AllocateZeroPool (4);
  if (NameSeg == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (NameSeg, Name, NameLen);

  if (NameLen < 4) {
    SetMem (&NameSeg[NameLen], 4 - NameLen, '_');
  }

  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (Object != NULL) {
    if (!EFI_ERROR (Status)) {
      Object->Data      = NameSeg;
      Object->DataSize  = 4;
      Object->Completed = TRUE;
    } else {
      InternalFreeAmlObject (&Object, ListHead);
      FreePool (NameSeg);
    }
  }

  return Status;
}

/**
  Creates a Namestring AML Object and inserts it into the linked list

  LeadNameChar      := 'A'-'Z' | '_'
  DigitChar         := '0'-'9'
  NameChar          := DigitChar | LeadNameChar
  RootChar          := '\'
  ParentPrefixChar  := '^'

  'A'-'Z'           := 0x41 - 0x5A
  '_'               := 0x5F
  '0'-'9'           := 0x30 - 0x39
  '\'               := 0x5C
  '^'               := 0x5E

  NameSeg           := <LeadNameChar NameChar NameChar NameChar>
                      // Notice that NameSegs shorter than 4 characters are filled with
                      // trailing underscores ('_'s).
  NameString        := <RootChar NamePath> | <PrefixPath NamePath>
  PrefixPath        := Nothing | <'^' PrefixPath>
  NamePath          := NameSeg | DualNamePath | MultiNamePath | NullName

  DualNamePath      := DualNamePrefix NameSeg NameSeg
  DualNamePrefix    := 0x2E
  MultiNamePath     := MultiNamePrefix SegCount NameSeg(SegCount)
  MultiNamePrefix   := 0x2F

  SegCount          := ByteData

  Note:SegCount can be from 1 to 255. For example: MultiNamePrefix(35) is
      encoded as 0x2f 0x23 and followed by 35 NameSegs. So, the total encoding
      length will be 1 + 1 + 35*4 = 142. Notice that: DualNamePrefix NameSeg
      NameSeg has a smaller encoding than the encoding of: MultiNamePrefix(2)
      NameSeg NameSeg

  SimpleName := NameString | ArgObj | LocalObj
  SuperName := SimpleName | DebugObj | Type6Opcode
  NullName := 0x00
  Target := SuperName | NullName

  @param[in]      String    - Null Terminated NameString Representation
  @param[in,out]  ListHead  - Head of Linked List of all AML Objects

  @return   EFI_SUCCESS     - Success
  @return   all others      - Fail
  **/
EFI_STATUS
EFIAPI
AmlOPNameString (
  IN      CHAR8       *String,
  IN OUT  LIST_ENTRY  *ListHead
  )
{
  EFI_STATUS           Status;
  AML_OBJECT_INSTANCE  *Object;
  CHAR8                *NameString;
  CHAR8                *NameStringPrefix;
  UINTN                NameStringBufferSize;
  UINTN                NameStringSize;
  UINTN                NameStringPrefixSize;
  UINTN                NameSegCount;
  UINTN                StringIndex;
  UINTN                StringLength;
  UINTN                NameSegIndex;
  BOOLEAN              FoundRootChar;
  BOOLEAN              FoundParentPrefixChar;
  BOOLEAN              FoundParenthesisOpenChar;
  BOOLEAN              FoundParenthesisCloseChar;

  if ((String == NULL) || (ListHead == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status                    = EFI_DEVICE_ERROR;
  Object                    = NULL;
  NameString                = NULL;
  FoundRootChar             = FALSE;
  FoundParentPrefixChar     = FALSE;
  NameStringBufferSize      = 0;
  FoundParenthesisOpenChar  = FALSE;
  FoundParenthesisCloseChar = FALSE;

  Status = InternalAppendNewAmlObjectNoData (&Object, ListHead);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Start NameString %a object\n", __FUNCTION__, String));
    goto Done;
  }

  // Create a buffer to fit NameSeg [4] * max NameSegCount [255]
  NameStringBufferSize = 4 * MAX_NAME_SEG_COUNT;
  NameString           = AllocateZeroPool (NameStringBufferSize);
  // Create arbitrarily large RootChar\ParentPrefixChar buffer
  NameStringPrefix = AllocateZeroPool (NameStringBufferSize);

  // Calculate length of required space
  StringLength         = AsciiStrLen (String);
  NameStringSize       = 0;
  NameStringPrefixSize = 0;
  NameSegIndex         = 0;
  NameSegCount         = 0;
  for (StringIndex = 0; StringIndex < StringLength; StringIndex++) {
    if (NameStringPrefixSize >= NameStringBufferSize) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "%a: ERROR: Exceeded ParentPrefixChar support at offset=%d of String=%a\n",
        __FUNCTION__,
        StringIndex,
        String
        ));
      goto Done;
    }

    if (InternalIsRootChar (String[StringIndex])) {
      if (NameSegCount != 0) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: RootChar at offset=%d of String=%a\n", __FUNCTION__, StringIndex, String));
        goto Done;
      }

      if (FoundRootChar) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: NameString=%a contains more than 1 RootChar.\n", __FUNCTION__, String));
        goto Done;
      }

      if (FoundParentPrefixChar) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: NameString=%a contains RootChar and ParentPrefixChar.\n", __FUNCTION__, String));
        goto Done;
      }

      // RootChar; increment NameStringSize
      NameStringPrefix[NameStringPrefixSize] = String[StringIndex];
      NameStringPrefixSize++;
      FoundRootChar = TRUE;
    } else if (InternalIsParentPrefixChar (String[StringIndex])) {
      if (NameSegCount != 0) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: ParentPrefixChar at offset=%d of String=%a\n", __FUNCTION__, StringIndex, String));
        goto Done;
      }

      if (FoundRootChar) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((DEBUG_ERROR, "%a: ERROR: NameString=%a contains RootChar and ParentPrefixChar.\n", __FUNCTION__, String));
        goto Done;
      }

      // ParentPrefixChar; increment NameStringSize
      NameStringPrefix[NameStringPrefixSize] = String[StringIndex];
      NameStringPrefixSize++;
      FoundParentPrefixChar = TRUE;
    } else if (!InternalIsNameChar (String[StringIndex])) {
      if (InternalIsNameSegSeparator (String[StringIndex])) {
        if (NameSegIndex == 0) {
          Status = EFI_INVALID_PARAMETER;
          DEBUG ((
            DEBUG_ERROR,
            "%a: ERROR: Invalid NameSeg separator at offset=%d of String=%a\n",
            __FUNCTION__,
            StringIndex,
            String
            ));
          goto Done;
        } else {
          NameSegIndex = 0;
        }
      } else if (String[StringIndex] == '(') {
        if (FoundParenthesisOpenChar) {
          Status = EFI_INVALID_PARAMETER;
          DEBUG ((
            DEBUG_ERROR,
            "%a: ERROR: Invalid Parenthesis at offset=%d of String=%a\n",
            __FUNCTION__,
            StringIndex,
            String
            ));
          goto Done;
        }

        FoundParenthesisOpenChar = TRUE;
      } else if (String[StringIndex] == ')') {
        if (FoundParenthesisCloseChar) {
          Status = EFI_INVALID_PARAMETER;
          DEBUG ((
            DEBUG_ERROR,
            "%a: ERROR: Invalid Parenthesis at offset=%d of String=%a\n",
            __FUNCTION__,
            StringIndex,
            String
            ));
          goto Done;
        } else if (!FoundParenthesisOpenChar) {
          Status = EFI_INVALID_PARAMETER;
          DEBUG ((
            DEBUG_ERROR,
            "%a: ERROR: No Open Parenthesis before offset=%d of String=%a\n",
            __FUNCTION__,
            StringIndex,
            String
            ));
          goto Done;
        }

        FoundParenthesisCloseChar = TRUE;
      } else {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: Unsupported character at offset=%d of String=%a\n",
          __FUNCTION__,
          StringIndex,
          String
          ));
        goto Done;
      }
    } else {
      // Must be NameChar
      if (FoundParenthesisOpenChar || FoundParenthesisCloseChar) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: NameChar after Parenthesis at offset=%d of String=%a\n",
          __FUNCTION__,
          StringIndex,
          String
          ));
        goto Done;
      } else if ((NameSegIndex == 0) && InternalIsDigitChar (String[StringIndex])) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: must be LeadNameChar at offset=%d of String=%a'\n",
          __FUNCTION__,
          StringIndex,
          String
          ));
        goto Done;
      }

      if (NameSegIndex >= 4) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: NameSeg > 4 characters at offset=%d of String=%a'\n",
          __FUNCTION__,
          StringIndex,
          String
          ));
        goto Done;
      } else {
        if (NameSegIndex == 0) {
          NameSegCount++;
          if (NameSegCount > MAX_NAME_SEG_COUNT) {
            Status = EFI_INVALID_PARAMETER;
            DEBUG ((
              DEBUG_ERROR,
              "%a: ERROR: Max NameSegCount=%d reached at offset=%d of String=%a'\n",
              __FUNCTION__,
              MAX_NAME_SEG_COUNT,
              StringIndex,
              String
              ));
            goto Done;
          }
        }

        NameString[NameStringSize] = String[StringIndex];
        NameStringSize++;
        NameSegIndex++;
        if ((StringIndex + 1 >= StringLength) ||
            !InternalIsNameChar (String[StringIndex + 1]))
        {
          // Extend in progress NameSeg with '_'s
          if (NameSegIndex < 4) {
            SetMem (&NameString[NameStringSize], 4 - NameSegIndex, '_');
            NameStringSize += 4 - NameSegIndex;
          }
        }
      }
    }
  }

  // Create AML Record with NameString contents from above
  // Copy in RootChar or ParentPrefixChar(s)
  if (NameStringPrefixSize != 0) {
    Object->Data = ReallocatePool (
                     Object->DataSize,
                     NameStringPrefixSize,
                     Object->Data
                     );
    CopyMem (
      &Object->Data[Object->DataSize],
      NameStringPrefix,
      NameStringPrefixSize
      );
    Object->DataSize += NameStringPrefixSize;
    FreePool (NameStringPrefix);
  }

  // Set up for Dual/MultiName Prefix
  if (NameSegCount > MAX_NAME_SEG_COUNT) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Exceeded MaxNameSegCount in NameString=%a\n", __FUNCTION__, String));
    goto Done;
  } else if (NameSegCount == 0) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Must be at least one NameSeg in NameString=%a\n", __FUNCTION__, String));
    goto Done;
  } else if (NameSegCount == 1) {
    // Single NameSeg
    Object->Data = ReallocatePool (
                     Object->DataSize,
                     Object->DataSize + NameStringSize,
                     Object->Data
                     );
  } else if (NameSegCount == 2) {
    Object->Data = ReallocatePool (
                     Object->DataSize,
                     Object->DataSize + NameStringSize + 1,
                     Object->Data
                     );
    Object->Data[Object->DataSize] = AML_DUAL_NAME_PREFIX;
    Object->DataSize              += 1;
  } else {
    Object->Data = ReallocatePool (
                     Object->DataSize,
                     Object->DataSize + NameStringSize + 2,
                     Object->Data
                     );
    Object->Data[Object->DataSize]     = AML_MULTI_NAME_PREFIX;
    Object->Data[Object->DataSize + 1] = NameSegCount & 0xFF;
    Object->DataSize                  += 2;
  }

  // Copy NameString data over. From above must be at least one NameSeg
  CopyMem (&Object->Data[Object->DataSize], NameString, NameStringSize);
  Object->DataSize += NameStringSize;
  FreePool (NameString);
  Object->Completed = TRUE;

  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    InternalFreeAmlObject (&Object, ListHead);
    if (NameString != NULL) {
      FreePool (NameString);
    }
  }

  return Status;
}
