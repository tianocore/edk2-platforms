/** @file
  AMD implementation of interfaces function for EFI_HII_CONFIG_ROUTING_PROTOCOL.
  This file provides better performance of BlockToConfig and ConfigToBlock
  functions.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AmdHiiConfigRouting.h"

HII_ELEMENT  gElementInfo[] = {
  { L"GUID=",   FIXED_STR_LEN (L"GUID=")   },
  { L"NAME=",   FIXED_STR_LEN (L"NAME=")   },
  { L"PATH=",   FIXED_STR_LEN (L"PATH=")   },
  { L"OFFSET=", FIXED_STR_LEN (L"OFFSET=") },
  { L"WIDTH=",  FIXED_STR_LEN (L"WIDTH=")  },
  { L"VALUE=",  FIXED_STR_LEN (L"VALUE=")  }
};

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param ConfigString  String to be converted

**/
VOID
EFIAPI
HiiToLower (
  IN EFI_STRING  ConfigString
  )
{
  EFI_STRING  String;
  BOOLEAN     Lower;

  ASSERT (ConfigString != NULL);

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && (*String >= L'A') && (*String <= L'F')) {
      *String = (CHAR16)(*String - L'A' + L'a');
    }
  }

  return;
}

//
// Updated EDK2 functions to improve performance.
//

/**
  Returns the length of a Null-terminated Unicode string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String.

  @param  String  A pointer to a Null-terminated Unicode string.

  @retval The length of String.

**/
UINTN
EFIAPI
HiiStrLen (
  IN EFI_STRING  String
  )
{
  UINTN  Length;

  ASSERT (String != NULL);

  for (Length = 0; String[Length] != L'\0'; Length++) {
  }

  //
  // If PcdMaximumUnicodeStringLength is not zero,
  // length should not more than PcdMaximumUnicodeStringLength
  //
  if (PcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
    ASSERT (Length < PcdGet32 (PcdMaximumUnicodeStringLength));
  }

  return Length;
}

/**
  Compares up to a specified length the contents of two Null-terminated Unicode
  strings, and returns the difference between the first mismatched Unicode
  characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. At most, Length Unicode
  characters will be compared. If Length is 0, then 0 is returned. If
  FirstString is identical to SecondString, then 0 is returned. Otherwise, the
  value returned is the first mismatched Unicode character in SecondString
  subtracted from the first mismatched Unicode character in FirstString.

  @param[in]  FirstString   A pointer to a Null-terminated Unicode string.
  @param[in]  SecondString  A pointer to a Null-terminated Unicode string.
  @param[in]  Length        The maximum number of Unicode characters to compare.

  @retval 0      FirstString is identical to SecondString.
  @retval others FirstString is not identical to SecondString.

**/
INTN
EFIAPI
HiiStrnCmp (
  IN EFI_STRING  FirstString,
  IN EFI_STRING  SecondString,
  IN UINTN       Length
  )
{
  if (Length == 0) {
    return 0;
  }

  ASSERT (FirstString != NULL);
  ASSERT (SecondString != NULL);
  if (PcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
    ASSERT (Length <= PcdGet32 (PcdMaximumUnicodeStringLength));
  }

  while ((*FirstString != L'\0') &&
         (*SecondString != L'\0') &&
         (*FirstString == *SecondString) &&
         (Length > 1))
  {
    FirstString++;
    SecondString++;
    Length--;
  }

  return *FirstString - *SecondString;
}

/**
  Initializes HII_NUMBER instance to 0.

  @param[in, out]  This    Pointer to HII_NUMBER instances.

**/
VOID
HiiNumberInit (
  IN OUT HII_NUMBER  *This
  )
{
  ASSERT (This != NULL);

  This->NumberPtr         = 0;
  This->NumberPtrLength   = 0;
  This->Value             = 0;
  This->PrivateBufferSize = 0;
}

/**
  Frees buffer in HII_NUMBER instance.

  @param[in, out]  This    Pointer to HII_NUMBER instance.

**/
VOID
HiiNumberFree (
  IN OUT HII_NUMBER  *This
  )
{
  ASSERT (This != NULL);

  if (This->NumberPtr != NULL) {
    FreePool (This->NumberPtr);
    This->NumberPtr         = NULL;
    This->PrivateBufferSize = 0;
  }
}

/**
  If buffer doesn't exist, allocate it. If the existing buffer is less than
  requested, allocate a larger one.

  @param[in, out]  This    Pointer to HII_NUMBER instance.
  @param[in]  Size    Requested buffer size.

  @retval EFI_SUCCESS           Buffer allocated.
  @retval EFI_OUT_OF_RESOURCES  OUt of memory.

**/
EFI_STATUS
HiiNumberSetMinBufferSize (
  IN OUT HII_NUMBER  *This,
  IN     UINTN       Size
  )
{
  ASSERT (This != NULL);

  if (This->PrivateBufferSize < Size) {
    Size           += MAX_STRING_LENGTH;
    This->NumberPtr = ReallocatePool (This->PrivateBufferSize, Size, This->NumberPtr);
    if (This->NumberPtr == NULL) {
      This->PrivateBufferSize = 0;
      return EFI_OUT_OF_RESOURCES;
    }

    This->PrivateBufferSize = Size;
  }

  return EFI_SUCCESS;
}

/**
  Get value of number from string and update HII_NUMBER instance.

  @param[in, out]  This    Pointer to HII_NUMBER instance.
  @param[in]  String  String to get value from. String may end in \0 or &.

  @retval EFI_SUCCESS           Buffer allocated.
  @retval EFI_OUT_OF_RESOURCES  OUt of memory.

**/
EFI_STATUS
GetValueOfNumber (
  IN OUT HII_NUMBER  *This,
  IN     EFI_STRING  String
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       StringLength;
  UINTN       NumLen;
  CHAR16      Digit;
  UINT8       DigitUint8;
  EFI_STRING  EndOfString;

  if ((This == NULL) || (String == NULL) || (*String == L'\0')) {
    return EFI_INVALID_PARAMETER;
  }

  EndOfString  = String;
  StringLength = 0;

  while (*EndOfString != L'\0' && *EndOfString != L'&') {
    EndOfString++;
    StringLength++;
  }

  This->StringLength = StringLength;

  NumLen = (StringLength + 1) / 2;

  Status = HiiNumberSetMinBufferSize (This, NumLen);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < StringLength; Index++) {
    Digit = String[StringLength - Index - 1];
    if (Digit < L'0') {
      DigitUint8 = 0;
    } else if (Digit <= L'9') {
      DigitUint8 = (UINT8)(Digit - L'0');
    } else if (Digit < L'A') {
      DigitUint8 = 0;
    } else if (Digit <= L'F') {
      DigitUint8 = (UINT8)(Digit - L'A' + 0xa);
    } else if (Digit < L'a') {
      DigitUint8 = 0;
    } else if (Digit <= L'f') {
      DigitUint8 = (UINT8)(Digit - L'a' + 0xa);
    } else {
      DigitUint8 = 0;
    }

    if ((Index & 1) == 0) {
      This->NumberPtr[Index / 2] = DigitUint8;
    } else {
      This->NumberPtr[Index / 2] = (UINT8)((DigitUint8 << 4) + This->NumberPtr[Index / 2]);
    }
  }

  This->NumberPtrLength = StringLength;
  This->Value           = 0;

  if (StringLength <= sizeof (UINTN) * sizeof (CHAR16)) {
    CopyMem (
      &This->Value,
      This->NumberPtr,
      NumLen < sizeof (UINTN) ? NumLen : sizeof (UINTN)
      );
  }

  return EFI_SUCCESS;
}

/**
  Initializes HII_STRING instance allocating buffer.

  @param[in, out]  This    Pointer to HII_STRING instance.
  @param[in]  Size    Size of initial allocation.

  @retval EFI_SUCCESS           Allocated buffer successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
**/
EFI_STATUS
HiiStringInit (
  IN OUT HII_STRING  *This,
  IN     UINTN       Size
  )
{
  ASSERT (This != NULL);

  This->StringLength = 0;

  if (Size == 0) {
    This->String            = NULL;
    This->PrivateBufferSize = 0;
    return EFI_SUCCESS;
  }

  This->String = (EFI_STRING)AllocatePool (Size);

  if (This->String != NULL) {
    This->String[0]         = L'\0';
    This->PrivateBufferSize = Size;
  } else {
    This->PrivateBufferSize = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Frees HiiString Buffer

  @param[in, out]  This    Pointer to HII_STRING instance.

**/
VOID
HiiStringFree (
  IN OUT HII_STRING  *This
  )
{
  ASSERT (This != NULL);

  if (This->String != NULL) {
    FreePool (This->String);
    This->String            = NULL;
    This->PrivateBufferSize = 0;
  }
}

/**
  If buffer doesn't exist, allocate it. If the existing buffer is less than
  requested, allocate a larger one.

  @param[in, out]  This    Pointer to HII_STRING instance.
  @param[in]  Size    Requested buffer size.

  @retval EFI_SUCCESS           Buffer allocated.
  @retval EFI_OUT_OF_RESOURCES  OUt of memory.

**/
EFI_STATUS
HiiStringSetMinBufferSize (
  IN OUT HII_STRING  *This,
  IN     UINTN       Size
  )
{
  UINTN       ThisStringSize;
  EFI_STRING  NewAlloc;

  ThisStringSize = (This->StringLength + 1) * sizeof (CHAR16);

  if (Size > This->PrivateBufferSize) {
    NewAlloc = (EFI_STRING)AllocatePool (Size);
    if (NewAlloc == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (NewAlloc, This->String, ThisStringSize);
    FreePool (This->String);
    This->String            = NewAlloc;
    This->PrivateBufferSize = Size;
  }

  return EFI_SUCCESS;
}

/**
  Append a string to the string in HII_STRING instance.

  @param[in, out]  This    Pointer to HII_STRING instance.
  @param[in]  String  String to append.

  @retval EFI_SUCCESS           String is appended.
  @retval EFI_OUT_OF_RESOURCES  OUt of memory.

**/
EFI_STATUS
HiiStringAppend (
  IN OUT HII_STRING  *This,
  IN     EFI_STRING  String
  )
{
  EFI_STATUS  Status;
  UINTN       ThisStringSize;
  UINTN       StringSize;
  UINTN       MaxLen;

  if ((This == NULL) || (String == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ThisStringSize = (This->StringLength + 1) * sizeof (CHAR16);
  StringSize     = HII_STR_SIZE (String);

  if (ThisStringSize + StringSize > This->PrivateBufferSize) {
    MaxLen = (ThisStringSize + StringSize) * 2;
    Status = HiiStringSetMinBufferSize (This, MaxLen);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Append the incoming string
  //
  CopyMem (&This->String[This->StringLength], String, StringSize);
  This->StringLength += StringSize / sizeof (CHAR16) - 1;

  return EFI_SUCCESS;
}

/**
  Append a number to the string in HII_STRING instance.

  @param[in, out]  This    Pointer to HII_STRING instance.
  @param[in]  Number  Number to append.
  @param[in]  Length  Length of Number.

  @retval EFI_SUCCESS           Number is appended.
  @retval EFI_OUT_OF_RESOURCES  OUt of memory.
**/
EFI_STATUS
HiiStringAppendValue (
  IN OUT  HII_STRING  *This,
  IN      UINT8       *Number,
  IN      UINTN       Length
  )
{
  EFI_STATUS  Status;
  UINTN       ThisStringSize;
  UINTN       Index;
  UINTN       Index2;
  UINT8       Nibble;
  UINTN       MaxLen;

  CHAR16  *String;

  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  ThisStringSize = (This->StringLength + 1) * sizeof (CHAR16);

  if (ThisStringSize + Length * 2 * sizeof (CHAR16) > This->PrivateBufferSize) {
    MaxLen = (ThisStringSize + Length * 2 * sizeof (CHAR16)) * 2; // Double requested string length.
    Status = HiiStringSetMinBufferSize (This, MaxLen);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  String              = This->String + This->StringLength;
  This->StringLength += Length * 2;

  Index = Length;

  do {
    Index--;
    Nibble = Number[Index] >> 4;

    for (Index2 = 0; Index2 < 2; Index2++) {
      if (Nibble < 0xa) {
        *String = '0' + Nibble;
      } else {
        *String = 'a' + Nibble - 0xa;
      }

      Nibble = Number[Index] & 0xf;
      String++;
    }
  } while (Index > 0);

  *String = '\0';

  return EFI_SUCCESS;
}

/**
  Find an element header in the input string, and return pointer it is value.

  This is a internal function.

  @param[in]  Hdr           Element Header to search for.
  @param[in]  String        Search for element header in this string.

  @retval Pointer to value in element header.
  @retval NULL if element header not found or end of string.

**/
EFI_STRING
FindElmentValue (
  IN ELEMENT_HDR  Hdr,
  IN EFI_STRING   String
  )
{
  ASSERT (String != NULL);

  if (HiiStrnCmp (String, gElementInfo[Hdr].ElementString, gElementInfo[Hdr].ElementLength) != 0) {
    return NULL;
  }

  return String + gElementInfo[Hdr].ElementLength;
}

/**
  Find pointer after value for element header in string.

  This is a internal function.

  @param[in]  String    String to search.

  @retval Pointer after value in element header.

**/
EFI_STRING
SkipElementValue (
  IN EFI_STRING  String
  )
{
  ASSERT (String != NULL);

  while (*String != 0 && *String != L'&') {
    String++;
  }

  if (*String == L'&') {
    String++;     // Skip '&'
  }

  return String;
}

/**
  Return pointer after ConfigHdr.

  This is a internal function.

  @param[in]  String String to search.

  @retval  Pointer after ConfigHdr.
  @retval  NULL if Config header not formed correctly.

**/
EFI_STRING
GetEndOfConfigHdr (
  IN EFI_STRING  String
  )
{
  ASSERT (String != NULL);

  String = FindElmentValue (ElementGuidHdr, String);
  if (String == NULL) {
    return NULL;
  }

  String = SkipElementValue (String);
  if (*String == 0) {
    return NULL;
  }

  while (*String != 0 &&
         HiiStrnCmp (
           String,
           gElementInfo[ElementPathHdr].ElementString,
           gElementInfo[ElementPathHdr].ElementLength
           )
         != 0)
  {
    String++;
  }

  if (*String != 0) {
    String = String + gElementInfo[ElementPathHdr].ElementLength;
  }

  String = SkipElementValue (String);
  return String;
}

/**
  This helper function is to be called by drivers to map configuration data
  stored in byte array ("block") formats such as UEFI Variables into current
  configuration strings.

  @param[in]  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param[in]  ConfigRequest          A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param[in]  Block                  Array of bytes defining the block's configuration.
  @param[in]  BlockSize              Length in bytes of Block.
  @param[out]  Config                 Filled-in configuration string. String allocated
                                 by the function. Returned only if call is
                                 successful. It is <ConfigResp> string format.
  @param[out]  Progress               A pointer to a string filled in with the offset of
                                 the most recent & before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The request succeeded. Progress points to the null
                                 terminator at the end of the ConfigRequest
                                 string.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to allocate Config. Progress
                                 points to the first character of ConfigRequest.
  @retval EFI_INVALID_PARAMETER  Passing in a NULL for the ConfigRequest or
                                 Block parameter would result in this type of
                                 error. Progress points to the first character of
                                 ConfigRequest.
  @retval EFI_DEVICE_ERROR       Block not large enough. Progress undefined.
  @retval EFI_INVALID_PARAMETER  Encountered non <BlockName> formatted string.
                                 Block is left updated and Progress points at
                                 the "&" preceding the first non-<BlockName>.

**/
EFI_STATUS
EFIAPI
HiiBlockToConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       ConfigRequest,
  IN  CONST UINT8                            *Block,
  IN  CONST UINTN                            BlockSize,
  OUT EFI_STRING                             *Config,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS  Status;
  EFI_STRING  StringPtr;
  EFI_STRING  OrigPtr;
  CHAR16      CharBackup;
  UINTN       Offset;
  UINTN       Width;
  UINT8       *Value;
  HII_STRING  HiiString;
  HII_NUMBER  HiiNumber;

  if ((This == NULL) || (Progress == NULL) || (Config == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Block == NULL) || (ConfigRequest == NULL)) {
    *Progress = ConfigRequest;
    return EFI_INVALID_PARAMETER;
  }

  StringPtr = ConfigRequest;

  Status = HiiStringInit (&HiiString, MAX_STRING_LENGTH);
  if (EFI_ERROR (Status)) {
    *Progress = ConfigRequest;
    return Status;
  }

  HiiNumberInit (&HiiNumber);

  //
  // Jump <ConfigHdr>
  //
  StringPtr = GetEndOfConfigHdr (StringPtr);
  if (StringPtr == NULL) {
    //
    //  Invalid header.
    //
    *Progress = ConfigRequest;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (*StringPtr == L'\0') {
    *Progress = StringPtr;
    HiiStringAppend (&HiiString, ConfigRequest);
    HiiToLower (HiiString.String);

    //
    // Do not free HiiString.String with HiiStringFree;
    //
    *Config = HiiString.String;
    return EFI_SUCCESS;
  }

  //
  // Copy <ConfigHdr> and an additional '&' to <ConfigResp>
  //
  CharBackup   = StringPtr[0];
  StringPtr[0] = L'\0';         // Temporarily change & to L'\0'
  Status       = HiiStringAppend (&HiiString, ConfigRequest);
  if (EFI_ERROR (Status)) {
    *Progress = ConfigRequest;
    goto Exit;
  }

  StringPtr[0] = CharBackup;

  //
  // Parse each <RequestElement> if exists
  // Only <BlockName> format is supported by this help function.
  // <BlockName> ::= 'OFFSET='<Number>&'WIDTH='<Number>
  //

  //
  // while search for "OFFSET="
  // When "OFFSET=" is found, OrigPtr starts at "OFFSET=", and StringPtr points to value.
  //
  while (*StringPtr != 0 &&
         (OrigPtr = StringPtr, (StringPtr = FindElmentValue (ElementOffsetHdr, StringPtr)) != NULL)
         )
  {
    //
    // Get Offset
    //
    Status = GetValueOfNumber (&HiiNumber, StringPtr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_OUT_OF_RESOURCES) {
        *Progress = ConfigRequest;  // Out of memory
      } else {
        *Progress = OrigPtr - 1;
      }

      goto Exit;
    }

    Offset     = HiiNumber.Value;
    StringPtr += HiiNumber.StringLength + 1;

    //
    // Get Width
    //
    StringPtr = FindElmentValue (ElementWidthHdr, StringPtr);
    if (StringPtr == NULL) {
      *Progress = OrigPtr - 1;
      Status    = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    Status = GetValueOfNumber (&HiiNumber, StringPtr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_OUT_OF_RESOURCES) {
        *Progress = ConfigRequest;  // Out of memory
      } else {
        *Progress = OrigPtr - 1;
      }

      goto Exit;
    }

    Width      = HiiNumber.Value;
    StringPtr += HiiNumber.StringLength;

    if ((*StringPtr != 0) && (*StringPtr != L'&')) {
      *Progress =  OrigPtr - 1;
      Status    = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    //
    // Calculate Value and convert it to hex string.
    //
    if (Offset + Width > BlockSize) {
      *Progress = StringPtr;
      Status    = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Value = (UINT8 *)Block + Offset;

    CharBackup = *StringPtr;
    *StringPtr = L'\0';

    Status = HiiStringAppend (&HiiString, OrigPtr);
    if (EFI_ERROR (Status)) {
      *Progress = ConfigRequest;  // Out of memory
      goto Exit;
    }

    *StringPtr = CharBackup;  // End of section of string OrigPtr

    Status = HiiStringAppend (&HiiString, L"&VALUE=");
    if (EFI_ERROR (Status)) {
      *Progress = ConfigRequest;  // Out of memory
      goto Exit;
    }

    Status = HiiStringAppendValue (&HiiString, Value, Width);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_OUT_OF_RESOURCES) {
        *Progress = ConfigRequest;  // Out of memory
      } else {
        *Progress =  OrigPtr - 1;
      }

      goto Exit;
    }

    //
    // If L'\0', parsing is finished. Otherwise skip L'&' to continue
    //
    if (*StringPtr == L'\0') {
      break;
    }

    Status = HiiStringAppend (&HiiString, L"&");
    if (EFI_ERROR (Status)) {
      *Progress = ConfigRequest;  // Out of memory
      goto Exit;
    }

    StringPtr++;  // Skip L'&'
  }

  if (*StringPtr != L'\0') {
    *Progress = StringPtr - 1;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  HiiToLower (HiiString.String);
  *Progress = StringPtr;

  HiiNumberFree (&HiiNumber);

  //
  // Do not free HiiString.String with HiiStringFree. The caller will
  // consume it when EFI_SUCCESS.
  //
  *Config = HiiString.String;

  return EFI_SUCCESS;

Exit:
  HiiStringFree (&HiiString);
  HiiNumberFree (&HiiNumber);

  *Config = NULL;

  return Status;
}

/**
  This helper function is to be called by drivers to map configuration strings
  to configurations stored in byte array ("block") formats such as UEFI Variables.

  @param[in]  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param[in]  ConfigResp             A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param[in, out]  Block                  A possibly null array of bytes representing the
                                 current block. Only bytes referenced in the
                                 ConfigResp string in the block are modified. If
                                 this parameter is null or if the *BlockSize
                                 parameter is (on input) shorter than required by
                                 the Configuration string, only the BlockSize
                                 parameter is updated and an appropriate status
                                 (see below) is returned.
  @param[in, out]  BlockSize              The length of the Block in units of UINT8. On
                                 input, this is the size of the Block. On output,
                                 if successful, contains the largest index of the
                                 modified byte in the Block, or the required buffer
                                 size if the Block is not large enough.
  @param[out]  Progress               On return, points to an element of the ConfigResp
                                 string filled in with the offset of the most
                                 recent '&' before the first failing name / value
                                 pair (or the beginning of the string if the
                                 failure is in the first name / value pair) or the
                                 terminating NULL if all was successful.

  @retval EFI_SUCCESS            The request succeeded. Progress points to the null
                                 terminator at the end of the ConfigResp string.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to allocate Config. Progress
                                 points to the first character of ConfigResp.
  @retval EFI_INVALID_PARAMETER  Passing in a NULL for the ConfigResp or
                                 Block parameter would result in this type of
                                 error. Progress points to the first character of
                                 ConfigResp.
  @retval EFI_INVALID_PARAMETER  Encountered non <BlockName> formatted name /
                                 value pair. Block is left updated and
                                 Progress points at the '&' preceding the first
                                 non-<BlockName>.
  @retval EFI_BUFFER_TOO_SMALL   Block not large enough. Progress undefined.
                                 BlockSize is updated with the required buffer size.
  @retval EFI_NOT_FOUND          Target for the specified routing data was not found.
                                 Progress points to the "G" in "GUID" of the errant
                                 routing data.

**/
EFI_STATUS
EFIAPI
HiiConfigToBlock (
  IN     CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN     CONST EFI_STRING                       ConfigResp,
  IN OUT UINT8                                  *Block,
  IN OUT UINTN                                  *BlockSize,
  OUT    EFI_STRING                             *Progress
  )
{
  EFI_STATUS  Status;
  EFI_STRING  StringPtr;
  EFI_STRING  OrigPtr;
  UINTN       Offset;
  UINTN       Width;
  UINTN       BufferSize;
  UINTN       MaxBlockSize;
  HII_NUMBER  HiiNumber;

  if ((This == NULL) || (BlockSize == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = ConfigResp;
  if (ConfigResp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringPtr    = ConfigResp;
  BufferSize   = *BlockSize;
  MaxBlockSize = 0;

  HiiNumberInit (&HiiNumber);
  //
  // Jump <ConfigHdr>
  //
  StringPtr = GetEndOfConfigHdr (StringPtr);
  if (StringPtr == NULL) {
    //
    //  Invalid header.
    //
    *Progress = ConfigResp;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (*StringPtr == L'\0') {
    *Progress = StringPtr;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Parse each <ConfigElement> if exists
  // Only '&'<BlockConfig> format is supported by this help function.
  // <BlockConfig> ::= 'OFFSET='<Number>&'WIDTH='<Number>&'VALUE='<Number>
  //
  while (*StringPtr != L'\0' &&
         (OrigPtr = StringPtr, (StringPtr = FindElmentValue (ElementOffsetHdr, StringPtr)) != NULL)
         )
  {
    //
    // Get Offset
    //
    Status = GetValueOfNumber (&HiiNumber, StringPtr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_OUT_OF_RESOURCES) {
        *Progress = ConfigResp;  // Out of memory
      } else {
        *Progress = OrigPtr - 1;
      }

      goto Exit;
    }

    Offset     = HiiNumber.Value;
    StringPtr += HiiNumber.StringLength + 1;

    //
    // Get Width
    //
    StringPtr = FindElmentValue (ElementWidthHdr, StringPtr);
    if (StringPtr == NULL) {
      *Progress = OrigPtr - 1;
      Status    = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    Status = GetValueOfNumber (&HiiNumber, StringPtr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_OUT_OF_RESOURCES) {
        *Progress = ConfigResp;  // Out of memory
      } else {
        *Progress = OrigPtr - 1;
      }

      goto Exit;
    }

    Width      = HiiNumber.Value;
    StringPtr += HiiNumber.StringLength + 1;

    //
    // Get Value
    //
    StringPtr = FindElmentValue (ElementValueHdr, StringPtr);
    if (StringPtr == NULL) {
      *Progress = OrigPtr - 1;
      Status    = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    Status = GetValueOfNumber (&HiiNumber, StringPtr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_OUT_OF_RESOURCES) {
        *Progress = ConfigResp;  // Out of memory
      } else {
        *Progress = OrigPtr - 1;
      }

      goto Exit;
    }

    //
    // Update the Block with configuration info
    //
    if ((Block != NULL) && (Offset + Width <= BufferSize)) {
      CopyMem (Block + Offset, HiiNumber.NumberPtr, Width);
    }

    if (Offset + Width > MaxBlockSize) {
      MaxBlockSize = Offset + Width;
    }

    StringPtr += HiiNumber.StringLength;

    if ((*StringPtr != L'\0') && (*StringPtr != L'&')) {
      *Progress = OrigPtr - 1;
      Status    = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    //
    // If L'\0', parsing is finished.
    //
    if (*StringPtr == L'\0') {
      break;
    }

    StringPtr++;  // Skip L'&'
  }

  //
  // The input string is not ConfigResp format, return error.
  //
  if (*StringPtr != L'\0') {
    *Progress = StringPtr;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  *Progress  = StringPtr + HiiStrLen (StringPtr);
  *BlockSize = MaxBlockSize - 1;

  if (MaxBlockSize > BufferSize) {
    *BlockSize = MaxBlockSize;
    if (Block != NULL) {
      Status = EFI_BUFFER_TOO_SMALL;
      goto Exit;
    }
  }

  if (Block == NULL) {
    *Progress = ConfigResp;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Status = EFI_SUCCESS;

Exit:

  HiiNumberFree (&HiiNumber);

  return Status;
}
