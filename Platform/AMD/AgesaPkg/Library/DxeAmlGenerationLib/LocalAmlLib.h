/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_AML_LIB_H_
#define INTERNAL_AML_LIB_H_

#include <Uefi.h>
#include <Library/AmlGenerationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Acpi.h>

#include "LocalAmlObjects.h"

// EDK2 open source MdePkg/Include/IndustryStandard/AcpiAml.h does not have
// these and should.
#define AML_DIGIT_CHAR_0  0x30
#define AML_DIGIT_CHAR_9  0x39

// The max string size for a QWord is 8 bytes = 16 characters plus NULL Terminator
#define MAX_AML_DATA_INTEGER_SIZE  17

// Defines similar to ctype.h functions isalpha() and isdigit()
#define IS_ASCII_UPPER_ALPHA(c)  ( ((c) >= AML_NAME_CHAR_A) && ((c) <= AML_NAME_CHAR_Z) )
#define IS_ASCII_HEX_DIGIT(c)    ( (((c) >= AML_DIGIT_CHAR_0) && ((c) <= AML_DIGIT_CHAR_9)) ||\
                                  (((c) >= AML_NAME_CHAR_A) && ((c) <= AML_NAME_CHAR_F)) )

// Swap bytes of upper and lower WORDs within a DWORD
#define Swap4Bytes(val) \
 ( (((val) >> 8) & 0x000000FF) | (((val) <<  8) & 0x0000FF00) | \
   (((val) >>  8) & 0x00FF0000) | (((val) << 8) & 0xFF000000) )

/*
  Calculates the optimized integer value used by AmlDataInteger and others

  Not a public function so no doxygen comment identifiers.

  @param[in]    Integer         - Integer value to encode
  @param[out]   ReturnData      - Allocated DataBuffer with encoded integer
  @param[out]   ReturnDataSize  - Size of ReturnData

  @return       EFI_SUCCESS     - Successful completion
  @return       EFI_OUT_OF_RESOURCES - Failed to allocate ReturnDataBuffer
*/
EFI_STATUS
EFIAPI
InternalAmlDataIntegerBuffer (
  IN      UINT64  Integer,
  OUT     VOID    **ReturnData,
  OUT     UINTN   *ReturnDataSize
  );

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
  );

/**
  Creates a NameSeg AML object and inserts it into the List

  NameSeg := <leadnamechar namechar namechar namechar>

  NameSegs shorter than 4 characters are filled with trailing underscores

  @param[in]      Name          - Field NameSeg
  @param[in,out]  ListHead      - Linked list has NameSeg after call

  @retval         EFI_SUCCESS
  @retval         Error status
**/
EFI_STATUS
EFIAPI
InternalAmlNameSeg (
  IN      CHAR8       *Name,
  IN OUT  LIST_ENTRY  *ListHead
  );

#endif // INTERNAL_AML_LIB_H_
